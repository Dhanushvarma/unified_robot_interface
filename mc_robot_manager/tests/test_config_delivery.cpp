#include <gtest/gtest.h>

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace
{

// Path constants — adjust to match your setup
const char * MCFLEET_BIN = "MCFleetControl";
const char * MCLOCAL_BIN = "mc_local";
const char * MCFLEET_CONFIG = "/home/vscode/workspace/sandbox/fleet/mc_robot_manager/tests/etc/mc_rtc.yaml";
const char * LOCAL_CONFIG_1 = "/home/vscode/workspace/sandbox/fleet/local_robot/etc/communication_1.yaml";
const char * LOCAL_CONFIG_2 = "/home/vscode/workspace/sandbox/fleet/local_robot/etc/communication_2.yaml";

class ProcessRunner
{
public:
  ProcessRunner(const std::string & bin, const std::vector<std::string> & args, const std::string & stdout_file)
  {
    pid_ = fork();
    if(pid_ == 0)
    {
      // Redirect stdout and stderr to the log file
      FILE * f = freopen(stdout_file.c_str(), "w", stdout);
      (void)f;
      dup2(fileno(stdout), fileno(stderr));

      std::vector<char *> argv;
      argv.push_back(const_cast<char *>(bin.c_str()));
      for(const auto & a : args)
      {
        argv.push_back(const_cast<char *>(a.c_str()));
      }
      argv.push_back(nullptr);

      execvp(bin.c_str(), argv.data());
      std::perror("exec failed");
      _exit(127);
    }
  }

  ~ProcessRunner()
  {
    if(pid_ > 0)
    {
      kill(pid_, SIGTERM);
      int status = 0;
      waitpid(pid_, &status, 0);
    }
  }

  ProcessRunner(const ProcessRunner &) = delete;
  ProcessRunner & operator=(const ProcessRunner &) = delete;

  bool isAlive() const
  {
    if(pid_ <= 0) return false;
    int status = 0;
    return waitpid(pid_, &status, WNOHANG) == 0;
  }

  pid_t pid() const
  {
    return pid_;
  }

private:
  pid_t pid_ = -1;
};

bool grepLogFile(const std::string & path, const std::string & pattern)
{
  std::ifstream f(path);
  if(!f.is_open()) return false;

  std::string line;
  while(std::getline(f, line))
  {
    if(line.find(pattern) != std::string::npos)
    {
      return true;
    }
  }
  return false;
}

std::string readLogFile(const std::string & path)
{
  std::ifstream f(path);
  if(!f.is_open()) return "";
  std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  return content;
}

} // namespace

TEST(E2EConfigDelivery, SingleClientReceivesConfigFromServer)
{
  const std::string fleet_log = "/tmp/e2e_mcfleet_single.log";
  const std::string local_log = "/tmp/e2e_mclocal.log";

  // ── Start MCFleetControl first (it starts the Zenoh router) ──
  ProcessRunner fleet(MCFLEET_BIN, {"-f", MCFLEET_CONFIG}, fleet_log);

  // Give router + queryables time to come up
  std::this_thread::sleep_for(std::chrono::seconds(2));

  ASSERT_TRUE(fleet.isAlive()) << "MCFleetControl died during startup\n" << readLogFile(fleet_log);

  // ── Start mc_local ──
  ProcessRunner local(MCLOCAL_BIN, {"-f", LOCAL_CONFIG_1}, local_log);

  // Wait for the config exchange
  bool got_config = false;
  for(int i = 0; i < 50; ++i) // up to 5 seconds
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if(!local.isAlive())
    {
      FAIL() << "mc_local died before receiving config\n"
             << "--- mc_local log ---\n"
             << readLogFile(local_log) << "\n--- MCFleetController log ---\n"
             << readLogFile(fleet_log);
    }

    if(grepLogFile(local_log, "Got config from server"))
    {
      got_config = true;
      break;
    }
  }

  EXPECT_TRUE(got_config) << "Client never received config\n"
                          << "--- mc_local log ---\n"
                          << readLogFile(local_log) << "\n--- MCFleetController log ---\n"
                          << readLogFile(fleet_log);
}

TEST(E2EConfigDelivery, MultipleClientReceivesConfigFromServer)
{
  const std::string fleet_log = "/tmp/e2e_mcfleet_multi.log";
  const std::string local_log_1 = "/tmp/e2e_mclocal_1.log";
  const std::string local_log_2 = "/tmp/e2e_mclocal_2.log";

  // ── Start MCFleetControl ──
  ProcessRunner fleet(MCFLEET_BIN, {"-f", MCFLEET_CONFIG}, fleet_log);

  std::this_thread::sleep_for(std::chrono::seconds(2));

  ASSERT_TRUE(fleet.isAlive()) << "MCFleetControl died during startup\n" << readLogFile(fleet_log);

  // ── Start mc_local ──
  ProcessRunner local_1(MCLOCAL_BIN, {"-f", LOCAL_CONFIG_1}, local_log_1);
  ProcessRunner local_2(MCLOCAL_BIN, {"-f", LOCAL_CONFIG_2}, local_log_2);

  // Wait for the config exchange
  bool got_config_1 = false;
  bool got_config_2 = false;
  for(int i = 0; i < 50; ++i) // up to 5 seconds
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if(!local_1.isAlive() && !got_config_1)
    {
      FAIL() << "mc_local_1 died before receiving config\n"
             << "--- mc_local_1 log ---\n"
             << readLogFile(local_log_1) << "\n--- MCFleetController log ---\n"
             << readLogFile(fleet_log);
    }

    if(!local_2.isAlive() && !got_config_2)
    {
      FAIL() << "mc_local_2 died before receiving config\n"
             << "--- mc_local_2 log ---\n"
             << readLogFile(local_log_2) << "\n--- MCFleetController log ---\n"
             << readLogFile(fleet_log);
    }

    if(!got_config_1 && grepLogFile(local_log_1, "Got config from server"))
    {
      got_config_1 = true;
    }

    if(!got_config_2 && grepLogFile(local_log_2, "Got config from server"))
    {
      got_config_2 = true;
    }

    if(got_config_1 && got_config_2)
    {
      break;
    }
  }

  EXPECT_TRUE(got_config_1) << "Client 1 (Robot 1) never received config\n"
                            << "--- mc_local_1 log ---\n"
                            << readLogFile(local_log_1) << "\n--- MCFleetController log ---\n"
                            << readLogFile(fleet_log);
  EXPECT_TRUE(got_config_2) << "Client 2 (Robot 2) never received config\n"
                            << "--- mc_local_2 log ---\n"
                            << readLogFile(local_log_2) << "\n--- MCFleetController log ---\n"
                            << readLogFile(fleet_log);
}

#pragma once

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace test_helper
{

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
    stop();
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

  void stop()
  {
    if(pid_ > 0)
    {
      kill(pid_, SIGINT);
      int status = 0;
      waitpid(pid_, &status, 0);
      pid_ = -1;
    }
  }

private:
  pid_t pid_ = -1;
};

inline bool grepLogFile(const std::string & path, const std::string & pattern)
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

inline std::string readLogFile(const std::string & path)
{
  std::ifstream f(path);
  if(!f.is_open()) return "";
  std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  return content;
}

inline int countRunningProcesses(const std::string & name)
{
  std::string cmd = "pgrep -c " + name;
  FILE * pipe = popen(cmd.c_str(), "r");
  if(!pipe) return 0;

  char buf[32] = {0};
  if(!fgets(buf, sizeof(buf), pipe))
  {
    pclose(pipe);
    return 0;
  }
  pclose(pipe);
  return std::atoi(buf);
}

inline bool waitForLogPattern(const ProcessRunner & proc,
                              const std::string & log,
                              const std::string & pattern,
                              std::chrono::milliseconds timeout)
{
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  const auto poll = std::chrono::milliseconds(100);

  while(std::chrono::steady_clock::now() < deadline)
  {
    if(!proc.isAlive()) return false;
    if(grepLogFile(log, pattern)) return true;
    std::this_thread::sleep_for(poll);
  }
  return false;
}

inline bool waitForProcessCount(const std::string & name, int expected_min, std::chrono::milliseconds timeout)
{
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while(std::chrono::steady_clock::now() < deadline)
  {
    if(countRunningProcesses(name) >= expected_min) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return false;
}

inline bool waitForProcessGone(const std::string & name, std::chrono::milliseconds timeout)
{
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while(std::chrono::steady_clock::now() < deadline)
  {
    if(countRunningProcesses(name) == 0) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return false;
}

} // namespace test_helper

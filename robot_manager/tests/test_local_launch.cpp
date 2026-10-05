#include "test_helpers.h"

#include <gtest/gtest.h>

namespace
{

// TODO: update test. these commands and paths are outdated
const char * MANAGER_BIN = "uri";
const char * LOCAL_BIN = "mc_local";
const char * MANAGER_LAUNCH_CONFIG =
    "/home/vscode/workspace/sandbox/unified_robot_interface/robot_manager/tests/etc/mc_rtc_launch.yaml";

class LocalLaunchTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Clean any leftover from previous tests
    std::system(("pkill -9 -x " + std::string(LOCAL_BIN) + " || true").c_str());
    std::system(("pkill -9 -x " + std::string(MANAGER_BIN) + " || true").c_str());
  }

  void TearDown() override
  {
    std::system(("pkill -9 -x " + std::string(LOCAL_BIN) + " || true").c_str());
    std::system(("pkill -9 -x " + std::string(MANAGER_BIN) + " || true").c_str());
  }
};

} // namespace

TEST_F(LocalLaunchTest, StartsLocalWhenLaunchTrue)
{
  using namespace test_helper;

  ASSERT_EQ(countRunningProcesses(LOCAL_BIN), 0) << "Test environment not clean: local already running";

  const std::string manager_log = "/tmp/e2e_launch_manager.log";
  ProcessRunner manager(MANAGER_BIN, {"-f", MANAGER_LAUNCH_CONFIG}, manager_log);

  bool started = waitForLogPattern(manager, manager_log, "Started local", std::chrono::seconds(5));
  EXPECT_TRUE(started) << "uri did not launch local.\n"
                       << "--- uri log ---\n"
                       << readLogFile(manager_log);

  EXPECT_TRUE(waitForProcessCount(LOCAL_BIN, 1, std::chrono::seconds(5))) << "local process not visible in ps.\n"
                                                                          << "--- uri log ---\n"
                                                                          << readLogFile(manager_log);
}

TEST_F(LocalLaunchTest, ShutdownStopsChildLocal)
{
  using namespace test_helper;

  const std::string manager_log = "/tmp/e2e_launch_shutdown.log";
  ProcessRunner manager(MANAGER_BIN, {"-f", MANAGER_LAUNCH_CONFIG}, manager_log);

  ASSERT_TRUE(waitForProcessCount(LOCAL_BIN, 1, std::chrono::seconds(5))) << "local never started\n"
                                                                          << readLogFile(manager_log);

  manager.stop();

  EXPECT_TRUE(waitForProcessGone(LOCAL_BIN, std::chrono::seconds(5))) << "local still running after shutdown\n"
                                                                      << readLogFile(manager_log);
}

TEST_F(LocalLaunchTest, LaunchedLocalReceivesConfig)
{
  using namespace test_helper;

  const std::string manager_log = "/tmp/e2e_launch_config.log";
  ProcessRunner manager(MANAGER_BIN, {"-f", MANAGER_LAUNCH_CONFIG}, manager_log);

  ASSERT_TRUE(waitForProcessCount(LOCAL_BIN, 1, std::chrono::seconds(5))) << "local never started\n"
                                                                          << readLogFile(manager_log);

  bool got_config = waitForLogPattern(manager, manager_log, "Got config from server", std::chrono::seconds(5));
  EXPECT_TRUE(got_config) << "Launched local never received config.\n"
                          << "--- uri and local log ---\n"
                          << readLogFile(manager_log);
}

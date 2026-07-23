#include "test_helpers.h"

#include <gtest/gtest.h>

namespace
{

const char * MCFLEET_BIN = "MCFleetControl";
const char * MCLOCAL_BIN = "mc_local";
const char * MCFLEET_LAUNCH_CONFIG =
    "/home/vscode/workspace/sandbox/fleet/mc_robot_manager/tests/etc/mc_rtc_launch.yaml";

class LocalLaunchTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Clean any leftover from previous tests
    std::system(("pkill -9 -x " + std::string(MCLOCAL_BIN) + " || true").c_str());
    std::system(("pkill -9 -x " + std::string(MCFLEET_BIN) + " || true").c_str());
  }

  void TearDown() override
  {
    std::system(("pkill -9 -x " + std::string(MCLOCAL_BIN) + " || true").c_str());
    std::system(("pkill -9 -x " + std::string(MCFLEET_BIN) + " || true").c_str());
  }
};

} // namespace

TEST_F(LocalLaunchTest, StartsMcLocalWhenLaunchTrue)
{
  using namespace test_helper;

  ASSERT_EQ(countRunningProcesses(MCLOCAL_BIN), 0) << "Test environment not clean: mc_local already running";

  const std::string fleet_log = "/tmp/e2e_launch_fleet.log";
  ProcessRunner fleet(MCFLEET_BIN, {"-f", MCFLEET_LAUNCH_CONFIG}, fleet_log);

  bool started = waitForLogPattern(fleet, fleet_log, "Started mc_local", std::chrono::seconds(5));
  EXPECT_TRUE(started) << "MCFleetControl did not launch mc_local.\n"
                       << "--- MCFleetControl log ---\n"
                       << readLogFile(fleet_log);

  EXPECT_TRUE(waitForProcessCount(MCLOCAL_BIN, 1, std::chrono::seconds(5))) << "mc_local process not visible in ps.\n"
                                                                            << "--- MCFleetControl log ---\n"
                                                                            << readLogFile(fleet_log);
}

TEST_F(LocalLaunchTest, ShutdownStopsChildMcLocal)
{
  using namespace test_helper;

  const std::string fleet_log = "/tmp/e2e_launch_shutdown.log";
  ProcessRunner fleet(MCFLEET_BIN, {"-f", MCFLEET_LAUNCH_CONFIG}, fleet_log);

  ASSERT_TRUE(waitForProcessCount(MCLOCAL_BIN, 1, std::chrono::seconds(5))) << "mc_local never started\n"
                                                                            << readLogFile(fleet_log);

  fleet.stop();

  EXPECT_TRUE(waitForProcessGone(MCLOCAL_BIN, std::chrono::seconds(5))) << "mc_local still running after shutdown\n"
                                                                        << readLogFile(fleet_log);
}

TEST_F(LocalLaunchTest, LaunchedMcLocalReceivesConfig)
{
  using namespace test_helper;

  const std::string fleet_log = "/tmp/e2e_launch_config.log";
  ProcessRunner fleet(MCFLEET_BIN, {"-f", MCFLEET_LAUNCH_CONFIG}, fleet_log);

  ASSERT_TRUE(waitForProcessCount(MCLOCAL_BIN, 1, std::chrono::seconds(5))) << "mc_local never started\n"
                                                                            << readLogFile(fleet_log);

  bool got_config = waitForLogPattern(fleet, fleet_log, "Got config from server", std::chrono::seconds(5));
  EXPECT_TRUE(got_config) << "Launched mc_local never received config.\n"
                          << "--- MCFleetControl and mc_local log ---\n"
                          << readLogFile(fleet_log);
}

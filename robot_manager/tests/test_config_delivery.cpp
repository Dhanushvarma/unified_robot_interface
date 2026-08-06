#include "test_helpers.h"

#include <gtest/gtest.h>

namespace
{

const char * MCFLEET_BIN = "MCFleetControl";
const char * MCLOCAL_BIN = "mc_local";
const char * MCFLEET_CONFIG = "/home/vscode/workspace/sandbox/fleet/mc_robot_manager/tests/etc/mc_rtc.yaml";
const char * LOCAL_CONFIG_1 = "/home/vscode/workspace/sandbox/fleet/local_robot/etc/communication_1.yaml";
const char * LOCAL_CONFIG_2 = "/home/vscode/workspace/sandbox/fleet/local_robot/etc/communication_2.yaml";

class ConfigDeliveryTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
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

TEST_F(ConfigDeliveryTest, SingleClientReceivesConfigFromServer)
{
  using namespace test_helper;

  const std::string fleet_log = "/tmp/e2e_mcfleet_single.log";
  const std::string local_log = "/tmp/e2e_mclocal.log";

  // ── Start MCFleetControl first (it starts the Zenoh router) ──
  ProcessRunner fleet(MCFLEET_BIN, {"-f", MCFLEET_CONFIG}, fleet_log);

  // Give router + queryables time to come up
  std::this_thread::sleep_for(std::chrono::seconds(2));

  ASSERT_TRUE(fleet.isAlive()) << "MCFleetControl died during startup\n" << readLogFile(fleet_log);

  // ── Start mc_local ──
  ProcessRunner local(MCLOCAL_BIN, {"-f", LOCAL_CONFIG_1}, local_log);

  bool got_config = waitForLogPattern(local, local_log, "Got config from server", std::chrono::seconds(5));
  EXPECT_TRUE(got_config) << "Client never received config\n"
                          << "--- mc_local log ---\n"
                          << readLogFile(local_log) << "\n--- MCFleetControl log ---\n"
                          << readLogFile(fleet_log);
}

TEST_F(ConfigDeliveryTest, MultipleClientReceivesConfigFromServer)
{
  using namespace test_helper;

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

  bool got_config_1 = waitForLogPattern(local_1, local_log_1, "Got config from server", std::chrono::seconds(30));
  bool got_config_2 = waitForLogPattern(local_2, local_log_2, "Got config from server", std::chrono::seconds(30));

  EXPECT_TRUE(got_config_1) << "Client 1 (Robot 1) never received config\n"
                            << "--- mc_local_1 log ---\n"
                            << readLogFile(local_log_1) << "\n--- MCFleetControl log ---\n"
                            << readLogFile(fleet_log);
  EXPECT_TRUE(got_config_2) << "Client 2 (Robot 2) never received config\n"
                            << "--- mc_local_2 log ---\n"
                            << readLogFile(local_log_2) << "\n--- MCFleetControl log ---\n"
                            << readLogFile(fleet_log);
}

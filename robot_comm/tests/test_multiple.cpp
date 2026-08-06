#include "config.h"
#include <robot_comm/CommunicationFactory.h>

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <future>
#include <iostream>
#include <thread>
#include <variant>

namespace fs = std::filesystem;

using namespace robot_comm;

TEST(CommunicationTest, ReliableMessageDeliveryAllMessages)
{
  mc_rtc::Configuration config(fs::path(TEST_CONFIG_DIR) / "etc/test.yaml");

  auto client =
      CommunicationFactory::makeCommunication("client", config("Robots")("robot1_zenoh")("network_interface"));

  auto server =
      CommunicationFactory::makeCommunication("server", config("Robots")("robot2_zenoh")("network_interface"));

  ASSERT_TRUE(client);
  ASSERT_TRUE(server);

  constexpr size_t NUM_MESSAGES = 1000;

  /// ------------------------------------------------------------
  /// Counters
  /// ------------------------------------------------------------

  std::atomic<size_t> config_received = 0;
  std::atomic<size_t> state_received = 0;
  std::atomic<size_t> command_received = 0;
  std::atomic<size_t> envelope_received = 0;

  constexpr size_t TOTAL_EXPECTED = NUM_MESSAGES * 4;

  std::atomic<size_t> total_received = 0;

  std::promise<void> done_promise;

  auto done_future = done_promise.get_future();

  auto notifyDone = [&]()
  {
    if(++total_received == TOTAL_EXPECTED)
    {
      done_promise.set_value();
    }
  };

  auto test_start = std::chrono::steady_clock::now();

  /// ------------------------------------------------------------
  /// CONFIG subscriber
  /// ------------------------------------------------------------

  auto config_sub = client->subscribe<std::string>("config",
                                                   [&](const std::string & msg)
                                                   {
                                                     ++config_received;

                                                     EXPECT_EQ(msg, "hello_config");

                                                     notifyDone();
                                                   });

  ASSERT_TRUE(config_sub);

  /// ------------------------------------------------------------
  /// STATE subscriber
  /// ------------------------------------------------------------

  auto state_sub = client->subscribe<State>("state",
                                            [&](const State & state)
                                            {
                                              ++state_received;

                                              ASSERT_EQ(state.position.size(), 3);

                                              EXPECT_DOUBLE_EQ(state.position[0], 1.0);

                                              notifyDone();
                                            });

  ASSERT_TRUE(state_sub);

  /// ------------------------------------------------------------
  /// COMMAND subscriber
  /// ------------------------------------------------------------

  auto command_sub = client->subscribe<Command>("command",
                                                [&](const Command & cmd)
                                                {
                                                  ++command_received;

                                                  EXPECT_DOUBLE_EQ(cmd.kp, 10.0);
                                                  EXPECT_DOUBLE_EQ(cmd.kd, 20.0);

                                                  ASSERT_EQ(cmd.position.size(), 3);

                                                  notifyDone();
                                                });

  ASSERT_TRUE(command_sub);

  /// ------------------------------------------------------------
  /// ENVELOPE subscriber
  /// ------------------------------------------------------------

  auto envelope_sub = client->subscribe<Envelope>("envelope",
                                                  [&](const Envelope & envelope)
                                                  {
                                                    ++envelope_received;

                                                    EXPECT_EQ(envelope.topic, "envelope");

                                                    ASSERT_TRUE(std::holds_alternative<Command>(envelope.payload));

                                                    const auto & cmd = std::get<Command>(envelope.payload);

                                                    EXPECT_DOUBLE_EQ(cmd.kp, 100.0);
                                                    EXPECT_DOUBLE_EQ(cmd.kd, 200.0);

                                                    notifyDone();
                                                  });

  ASSERT_TRUE(envelope_sub);

  // Add some delay for discovery
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  /// ------------------------------------------------------------
  /// Allow discovery/setup
  /// ------------------------------------------------------------

  auto publish_start = std::chrono::steady_clock::now();

  /// ------------------------------------------------------------
  /// Publish
  /// ------------------------------------------------------------

  for(size_t i = 0; i < NUM_MESSAGES; ++i)
  {
    /// CONFIG
    ASSERT_TRUE(server->publish("config", std::string("hello_config")));

    /// STATE
    State state;
    state.position = {1.0, 2.0, 3.0};
    state.velocity = {4.0, 5.0, 6.0};
    state.torque = {7.0, 8.0, 9.0};
    ASSERT_TRUE(server->publish("state", state));

    /// COMMAND

    Command command;
    command.kp = 10.0;
    command.kd = 20.0;
    command.position = {1.0, 2.0, 3.0};
    command.velocity = {4.0, 5.0, 6.0};
    command.torque = {7.0, 8.0, 9.0};
    ASSERT_TRUE(server->publish("command", command));

    /// ENVELOPE

    Envelope envelope;
    envelope.topic = "envelope";
    envelope.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());

    Command envelope_command;
    envelope_command.kp = 100.0;
    envelope_command.kd = 200.0;
    envelope_command.position = {1.0 * i, 2.0 * i, 3.0 * i};
    envelope.payload = envelope_command;

    ASSERT_TRUE(server->publish("envelope", envelope));

    std::cout << "I sent message id : " << i << std::endl;
  }

  auto publish_end = std::chrono::steady_clock::now();

  /// ------------------------------------------------------------
  /// Wait for completion
  /// ------------------------------------------------------------

  auto status = done_future.wait_for(std::chrono::seconds(1));

  auto test_end = std::chrono::steady_clock::now();

  ASSERT_EQ(status, std::future_status::ready);

  /// ------------------------------------------------------------
  /// Validate counts
  /// ------------------------------------------------------------

  EXPECT_EQ(config_received.load(), NUM_MESSAGES);

  EXPECT_EQ(state_received.load(), NUM_MESSAGES);

  EXPECT_EQ(command_received.load(), NUM_MESSAGES);

  EXPECT_EQ(envelope_received.load(), NUM_MESSAGES);

  /// ------------------------------------------------------------
  /// Stats
  /// ------------------------------------------------------------

  const auto publish_duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(publish_end - publish_start).count();

  const auto total_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(test_end - test_start).count();

  const double publish_seconds = std::max(publish_duration_us / 1e6, 1e-9);

  const double total_seconds = std::max(total_duration_us / 1e6, 1e-9);

  const double publish_rate = static_cast<double>(TOTAL_EXPECTED) / publish_seconds;

  const double end_to_end_rate = static_cast<double>(total_received.load()) / total_seconds;

  std::cout << "\n";
  std::cout << "====================================\n";
  std::cout << "Communication Statistics\n";
  std::cout << "====================================\n";
  std::cout << "Messages sent      : " << TOTAL_EXPECTED << "\n";
  std::cout << "Messages received  : " << total_received.load() << "\n";
  std::cout << "Publish time (us)  : " << publish_duration_us << "\n";
  std::cout << "Total time (us)    : " << total_duration_us << "\n";
  std::cout << "Publish rate       : " << publish_rate << " msg/s\n";
  std::cout << "End-to-end rate    : " << end_to_end_rate << " msg/s\n";
  std::cout << "====================================\n";
}

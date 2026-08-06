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

TEST(CommunicationTest, ReliableMessageDelivery)
{
  mc_rtc::Configuration config(fs::path(TEST_CONFIG_DIR) / "etc/test.yaml");

  auto client =
      CommunicationFactory::makeCommunication("client", config("Robots")("robot1_zenoh")("network_interface"));

  auto server =
      CommunicationFactory::makeCommunication("server", config("Robots")("robot2_zenoh")("network_interface"));

  ASSERT_TRUE(client);
  ASSERT_TRUE(server);

  constexpr size_t NUM_MESSAGES = 1000;

  std::atomic<size_t> received_count = 0;

  std::promise<void> done_promise;

  auto done_future = done_promise.get_future();

  auto test_start = std::chrono::steady_clock::now();

  /// ------------------------------------------------------------
  /// Subscriber
  /// ------------------------------------------------------------

  auto subscriber = client->subscribe<Envelope>("test",
                                                [&](const Envelope & envelope)
                                                {
                                                  auto count = ++received_count;

                                                  EXPECT_EQ(envelope.topic, "test");

                                                  ASSERT_TRUE(std::holds_alternative<State>(envelope.payload));

                                                  const auto & cmd = std::get<State>(envelope.payload);

                                                  // EXPECT_EQ(cmd.kd, 25);

                                                  if(count == NUM_MESSAGES)
                                                  {
                                                    done_promise.set_value();
                                                  }
                                                });

  ASSERT_TRUE(subscriber);

  // Add some delay for discovery
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  /// ------------------------------------------------------------
  /// Allow discovery/setup
  /// ------------------------------------------------------------

  auto publish_start = std::chrono::steady_clock::now();

  /// ------------------------------------------------------------
  /// Publish burst
  /// ------------------------------------------------------------

  for(size_t i = 0; i < NUM_MESSAGES; ++i)
  {
    Envelope envelope;

    envelope.topic = "test";

    envelope.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());

    State state;

    // command.kd = 25;

    state.position = {1.0 * i, 2.0 * i, 3.0 * i};

    envelope.payload = state;

    ASSERT_TRUE(server->publish("test", envelope));
  }

  auto publish_end = std::chrono::steady_clock::now();

  /// ------------------------------------------------------------
  /// Wait for completion
  /// ------------------------------------------------------------

  auto status = done_future.wait_for(std::chrono::seconds(10));

  auto test_end = std::chrono::steady_clock::now();

  ASSERT_EQ(status, std::future_status::ready);

  const auto received = received_count.load();

  EXPECT_EQ(received, NUM_MESSAGES);

  /// ------------------------------------------------------------
  /// Stats
  /// ------------------------------------------------------------

  const auto publish_duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(publish_end - publish_start).count();

  const auto total_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(test_end - test_start).count();

  const double publish_seconds = std::max(publish_duration_us / 1e6, 1e-9);

  const double total_seconds = std::max(total_duration_us / 1e6, 1e-9);

  const double publish_rate = static_cast<double>(NUM_MESSAGES) / publish_seconds;

  const double end_to_end_rate = static_cast<double>(received) / total_seconds;

  const size_t lost_messages = NUM_MESSAGES - received;

  std::cout << "\n";
  std::cout << "====================================\n";
  std::cout << "Communication Statistics\n";
  std::cout << "====================================\n";
  std::cout << "Messages sent      : " << NUM_MESSAGES << "\n";
  std::cout << "Messages received  : " << received << "\n";
  std::cout << "Messages lost      : " << lost_messages << "\n";
  std::cout << "Publish time (us) : " << publish_duration_us << "\n";
  std::cout << "Total time (us) : " << total_duration_us << "\n";
  std::cout << "Publish rate (msg/s): " << publish_rate << "\n";
  std::cout << "End-to-end rate (msg/s): " << end_to_end_rate << "\n";
  std::cout << "====================================\n";
}

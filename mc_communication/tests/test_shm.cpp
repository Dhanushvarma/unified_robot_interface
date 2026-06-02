#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <thread>
#include <variant>

#include <gtest/gtest.h>

#include <mc_communication/CommunicationFactory.h>

#include "config.h"

namespace fs = std::filesystem;

using namespace mc_communication;

TEST(CommunicationTest, SharedMemoryLatestMessage)
{
  mc_rtc::Configuration config(fs::path(TEST_CONFIG_DIR) / "test.yaml");

  auto client = CommunicationFactory::makeCommunication("client", config("Robots")("robot1_shm")("network_interface"));

  auto server = CommunicationFactory::makeCommunication("server", config("Robots")("robot2_shm")("network_interface"));

  ASSERT_TRUE(client);
  ASSERT_TRUE(server);

  // client->start();
  // server->start();

  constexpr size_t NUM_MESSAGES = 1000;

  std::atomic<bool> received = false;

  std::promise<void> done_promise;

  auto done_future = done_promise.get_future();

  size_t latest_index = 0;

  auto subscriber = client->subscribe<Envelope>("test",
                                                [&](const Envelope & envelope)
                                                {
                                                  ASSERT_TRUE(std::holds_alternative<Command>(envelope.payload));

                                                  const auto & cmd = std::get<Command>(envelope.payload);

                                                  latest_index = static_cast<size_t>(cmd.position[0]);

                                                  received = true;

                                                  if(latest_index == NUM_MESSAGES - 1)
                                                  {
                                                    done_promise.set_value();
                                                  }
                                                });

  ASSERT_TRUE(subscriber);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for(size_t i = 0; i < NUM_MESSAGES; ++i)
  {
    Envelope envelope;

    envelope.topic = "test";

    envelope.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());

    Command command;

    command.kd = 25;

    command.position = {static_cast<double>(i), 2.0 * i, 3.0 * i};

    envelope.payload = command;

    ASSERT_TRUE(server->publish("test", envelope));
  }

  auto status = done_future.wait_for(std::chrono::seconds(5));

  ASSERT_EQ(status, std::future_status::ready);

  ASSERT_TRUE(received);

  EXPECT_EQ(latest_index, NUM_MESSAGES - 1);

  // client->stop();
  // server->stop();
}

#include <chrono>
#include <filesystem>
#include <thread>
#include <variant>

#include <gtest/gtest.h>

#include <mc_communication/CommunicationFactory.h>

#include "config.h"

namespace fs = std::filesystem;

using namespace mc_communication;

TEST(QueryReplyTest, ConfigDelivery)
{
  mc_rtc::Configuration config(fs::path(TEST_CONFIG_DIR) / "test.yaml");

  // Create server and client on different robot topics
  auto server =
      CommunicationFactory::makeCommunication("server", config("Robots")("robot1_zenoh")("network_interface"));

  auto client =
      CommunicationFactory::makeCommunication("client", config("Robots")("robot2_zenoh")("network_interface"));

  ASSERT_TRUE(server);
  ASSERT_TRUE(client);

  // Server registers a queryable that returns a known config string
  const std::string expected = "hello config";

  server->registerQueryable<std::string>("config", [expected]() -> std::string { return expected; });

  // Give Zenoh time to propagate the queryable declaration
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Client queries the config
  auto received = client->query<std::string>("config", std::chrono::seconds(2));

  ASSERT_TRUE(received.has_value()) << "Query timed out — server didn't respond";
  EXPECT_EQ(*received, expected);
}

TEST(QueryReplyTest, QueryTimesOutIfNoQueryable)
{
  mc_rtc::Configuration config(fs::path(TEST_CONFIG_DIR) / "test.yaml");

  auto client =
      CommunicationFactory::makeCommunication("client", config("Robots")("robot1_zenoh")("network_interface"));

  ASSERT_TRUE(client);

  // No queryable registered anywhere → should time out
  auto received = client->query<std::string>("config", std::chrono::seconds(2));

  EXPECT_FALSE(received.has_value()) << "Expected timeout, got a value";
}

TEST(QueryReplyTest, MultipleQueriesReceiveSameConfig)
{
  mc_rtc::Configuration config(fs::path(TEST_CONFIG_DIR) / "test.yaml");

  auto server =
      CommunicationFactory::makeCommunication("server", config("Robots")("robot1_zenoh")("network_interface"));

  auto client =
      CommunicationFactory::makeCommunication("client", config("Robots")("robot2_zenoh")("network_interface"));

  const std::string expected = "hello_config";

  server->registerQueryable<std::string>("config", [expected]() -> std::string { return expected; });

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Query multiple times — each time we should get the same reply
  for(int i = 0; i < 5; ++i)
  {
    auto received = client->query<std::string>("config", std::chrono::seconds(2));
    ASSERT_TRUE(received.has_value()) << "Query " << i << " timed out";
    EXPECT_EQ(*received, expected) << "Query " << i << " returned wrong data";
  }
}

TEST(QueryReplyTest, LateJoiningClientCanStillGetConfig)
{
  mc_rtc::Configuration config(fs::path(TEST_CONFIG_DIR) / "test.yaml");

  // Server starts first and registers the queryable
  auto server =
      CommunicationFactory::makeCommunication("server", config("Robots")("robot1_zenoh")("network_interface"));

  const std::string expected = "config_registered_before_client";

  server->registerQueryable<std::string>("config", [expected]() -> std::string { return expected; });

  // Simulate a delay before the client joins
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Client joins late
  auto client =
      CommunicationFactory::makeCommunication("client", config("Robots")("robot2_zenoh")("network_interface"));

  ASSERT_TRUE(client);

  // Give Zenoh time to establish the session
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  auto received = client->query<std::string>("config", std::chrono::seconds(2));

  ASSERT_TRUE(received.has_value()) << "Late-joining client couldn't get config";
  EXPECT_EQ(*received, expected);
}

TEST(QueryReplyTest, LateJoiningServerCanStillSendConfig)
{
  mc_rtc::Configuration config(fs::path(TEST_CONFIG_DIR) / "test.yaml");

  // Client starts first
  auto client =
      CommunicationFactory::makeCommunication("client", config("Robots")("robot2_zenoh")("network_interface"));
  ASSERT_TRUE(client);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  auto received_1 = client->query<std::string>("config", std::chrono::seconds(2));

  EXPECT_FALSE(received_1.has_value()) << "Expected timeout, got a value";

  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Server joins late
  auto server =
      CommunicationFactory::makeCommunication("server", config("Robots")("robot1_zenoh")("network_interface"));
  const std::string expected = "Server is finally here.";
  server->registerQueryable<std::string>("config", [expected]() -> std::string { return expected; });
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  auto received_2 = client->query<std::string>("config", std::chrono::seconds(2));

  ASSERT_TRUE(received_2.has_value()) << "Late server couldn't reply to query config";
  EXPECT_EQ(*received_2, expected);
}

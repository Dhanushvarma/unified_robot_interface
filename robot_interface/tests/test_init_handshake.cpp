#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#include <gtest/gtest.h>

#include <robot_comm/CommunicationFactory.h>

using namespace robot_comm;

// Minimal Zenoh config for in-process tests (no router, no SHM).
static mc_rtc::Configuration zenoh_config()
{
  mc_rtc::Configuration c;
  c.add("protocol", std::string("zenoh"));
  return c;
}

// ─── Normal handshake ────────────────────────────────────────────────────────

TEST(InitHandshake, QueryReturnsOK)
{
  // robot_interface side: registers queryable on "{name}/init"
  auto client = CommunicationFactory::makeCommunication("handshake_robot", zenoh_config());
  client->setupClient();

  std::promise<ByteBuffer> received;
  client->handleQuery("handshake_robot/init",
                      [&](const ByteBuffer & payload) -> ByteBuffer
                      {
                        received.set_value(payload);
                        const std::string ok = "OK";
                        return ByteBuffer(ok.begin(), ok.end());
                      });

  // Let Zenoh discover the queryable before the server queries it.
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // robot_manager side: sends the init query
  auto server = CommunicationFactory::makeCommunication("handshake_robot", zenoh_config());
  server->setupServer();

  const std::string config_yaml = "robot_interface:\n  driver: DummyDriver\n  ip: 127.0.0.1\n  port: 6\n";
  ByteBuffer payload(config_yaml.begin(), config_yaml.end());

  auto reply = server->query("handshake_robot/init", payload, std::chrono::seconds(5));

  // Reply must arrive and equal "OK"
  ASSERT_TRUE(reply.has_value()) << "Init query timed out";
  const std::string reply_str(reply->begin(), reply->end());
  EXPECT_EQ(reply_str, "OK");

  // The queryable must have received the exact config payload
  auto fut = received.get_future();
  ASSERT_EQ(fut.wait_for(std::chrono::milliseconds(100)), std::future_status::ready);
  const ByteBuffer got = fut.get();
  const std::string got_str(got.begin(), got.end());
  EXPECT_EQ(got_str, config_yaml);
}

// ─── Error reply ─────────────────────────────────────────────────────────────

TEST(InitHandshake, QueryReturnsError)
{
  auto client = CommunicationFactory::makeCommunication("error_robot", zenoh_config());
  client->setupClient();

  client->handleQuery("error_robot/init",
                      [](const ByteBuffer &) -> ByteBuffer
                      {
                        const std::string err = "ERROR: driver not found";
                        return ByteBuffer(err.begin(), err.end());
                      });

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  auto server = CommunicationFactory::makeCommunication("error_robot", zenoh_config());
  server->setupServer();

  auto reply = server->query("error_robot/init", ByteBuffer{}, std::chrono::seconds(5));

  ASSERT_TRUE(reply.has_value());
  const std::string reply_str(reply->begin(), reply->end());
  EXPECT_EQ(reply_str.substr(0, 5), "ERROR");
}

// ─── Timeout when no robot_interface is running ──────────────────────────────

TEST(InitHandshake, QueryTimesOutWhenNoQueryable)
{
  auto server = CommunicationFactory::makeCommunication("ghost_robot", zenoh_config());
  server->setupServer();

  auto reply = server->query("ghost_robot/init", ByteBuffer{}, std::chrono::milliseconds(500));

  EXPECT_FALSE(reply.has_value()) << "Expected timeout but got a reply";
}

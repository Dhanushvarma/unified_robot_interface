#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include <robot_comm/CommunicationFactory.h>

using namespace robot_comm;

static mc_rtc::Configuration zenoh_config()
{
  mc_rtc::Configuration c;
  c.add("protocol", std::string("zenoh"));
  return c;
}

// ── Fixture ──────────────────────────────────────────────────────────────────
// Each test case gets a fresh server+client pair on a unique robot name so
// Zenoh subscriptions from one test don't bleed into another.

struct PubSubFixture : ::testing::Test
{
  std::unique_ptr<Communication> server; // robot_manager side
  std::unique_ptr<Communication> client; // robot_interface side

  void SetUp() override
  {
    // Build a unique name from the current test to avoid Zenoh topic collisions
    const auto * info = ::testing::UnitTest::GetInstance()->current_test_info();
    const std::string name = std::string("ps_") + info->name();

    server = CommunicationFactory::makeCommunication(name, zenoh_config());
    server->setupServer();

    client = CommunicationFactory::makeCommunication(name, zenoh_config());
    client->setupClient();

    // Give Zenoh time to wire up subscriptions before the test sends anything.
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }
};

// ── Helpers ───────────────────────────────────────────────────────────────────

// Poll `comm->receive()` until a non-empty buffer arrives or the deadline
// passes. Returns the buffer, or empty on timeout.
static ByteBuffer poll_receive(Communication & comm, std::chrono::milliseconds timeout = std::chrono::seconds(3))
{
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while(std::chrono::steady_clock::now() < deadline)
  {
    auto rx = comm.receive();
    if(rx && !rx->empty()) return *rx;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return {};
}

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST_F(PubSubFixture, ReceiveReturnsEmptyBeforeFirstMessage)
{
  // receive() must return an empty (not nullopt) buffer before any data arrives.
  auto rx = client->receive();
  ASSERT_TRUE(rx.has_value()) << "receive() returned nullopt — was setupClient() called?";
  EXPECT_TRUE(rx->empty());

  rx = server->receive();
  ASSERT_TRUE(rx.has_value());
  EXPECT_TRUE(rx->empty());
}

TEST_F(PubSubFixture, StateFlowsFromClientToServer)
{
  State state;
  state.position = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
  state.velocity = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  state.torque = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

  ASSERT_TRUE(client->send(client->encode(state)));

  const ByteBuffer buf = poll_receive(*server);
  ASSERT_FALSE(buf.empty()) << "Server did not receive state within timeout";

  auto decoded = server->serializer()->deserialize<State>(MessageType::STATE, buf.data(), buf.size());
  ASSERT_TRUE(decoded.has_value());

  ASSERT_EQ(decoded->position.size(), state.position.size());
  for(size_t i = 0; i < state.position.size(); ++i) EXPECT_NEAR(decoded->position[i], state.position[i], 1e-9);

  ASSERT_EQ(decoded->velocity.size(), state.velocity.size());
  for(size_t i = 0; i < state.velocity.size(); ++i) EXPECT_NEAR(decoded->velocity[i], state.velocity[i], 1e-9);
}

TEST_F(PubSubFixture, CommandFlowsFromServerToClient)
{
  Command cmd;
  cmd.kp = 120.0;
  cmd.kd = 12.0;
  cmd.position = {0.5, 0.4, 0.3, 0.2, 0.1, 0.0};
  cmd.velocity = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  cmd.torque = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

  ASSERT_TRUE(server->send(server->encode(cmd)));

  const ByteBuffer buf = poll_receive(*client);
  ASSERT_FALSE(buf.empty()) << "Client did not receive command within timeout";

  auto decoded = client->serializer()->deserialize<Command>(MessageType::COMMAND, buf.data(), buf.size());
  ASSERT_TRUE(decoded.has_value());

  EXPECT_NEAR(decoded->kp, cmd.kp, 1e-9);
  EXPECT_NEAR(decoded->kd, cmd.kd, 1e-9);

  ASSERT_EQ(decoded->position.size(), cmd.position.size());
  for(size_t i = 0; i < cmd.position.size(); ++i) EXPECT_NEAR(decoded->position[i], cmd.position[i], 1e-9);
}

TEST_F(PubSubFixture, BidirectionalExchange)
{
  // Robot interface publishes state; manager publishes command in the same cycle.
  State state;
  state.position = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  ASSERT_TRUE(client->send(client->encode(state)));

  Command cmd;
  cmd.kp = 50.0;
  cmd.kd = 5.0;
  cmd.position = {2.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  ASSERT_TRUE(server->send(server->encode(cmd)));

  const ByteBuffer state_buf = poll_receive(*server);
  ASSERT_FALSE(state_buf.empty());
  auto got_state = server->serializer()->deserialize<State>(MessageType::STATE, state_buf.data(), state_buf.size());
  ASSERT_TRUE(got_state.has_value());
  EXPECT_NEAR(got_state->position[0], 1.0, 1e-9);

  const ByteBuffer cmd_buf = poll_receive(*client);
  ASSERT_FALSE(cmd_buf.empty());
  auto got_cmd = client->serializer()->deserialize<Command>(MessageType::COMMAND, cmd_buf.data(), cmd_buf.size());
  ASSERT_TRUE(got_cmd.has_value());
  EXPECT_NEAR(got_cmd->position[0], 2.0, 1e-9);
}

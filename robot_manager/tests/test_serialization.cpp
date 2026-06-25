#include <mc_communication/Communication.h>
#include <gtest/gtest.h>
#include <random>

using namespace mc_communication;

// ============================================================
// 1. Simple roundtrip tests
// ============================================================

TEST(SerializationTest, StateRoundtrip)
{
  State original;
  original.position = {1.0, 2.0, 3.0};
  original.velocity = {4.0, 5.0, 6.0};
  original.torque = {7.0, 8.0, 9.0};

  auto buffer = Communication::serializeState(original);
  auto result = Communication::deserializeState(buffer.data(), buffer.size());

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->position, original.position);
  EXPECT_EQ(result->velocity, original.velocity);
  EXPECT_EQ(result->torque, original.torque);
}

TEST(SerializationTest, CommandRoundtrip)
{
  Command original;
  original.kp = 100.0;
  original.kd = 10.0;
  original.position = {0.1, 0.2, 0.3};
  original.velocity = {0.4, 0.5, 0.6};
  original.torque = {0.7, 0.8, 0.9};

  auto buffer = Communication::serializeCommand(original);
  auto result = Communication::deserializeCommand(buffer.data(), buffer.size());

  ASSERT_TRUE(result.has_value());
  EXPECT_DOUBLE_EQ(result->kp, original.kp);
  EXPECT_DOUBLE_EQ(result->kd, original.kd);
  EXPECT_EQ(result->position, original.position);
  EXPECT_EQ(result->velocity, original.velocity);
  EXPECT_EQ(result->torque, original.torque);
}

// ============================================================
// 2. Edge cases
// ============================================================

TEST(SerializationTest, DeserializeInvalidDataReturnsNullopt)
{
  std::array<uint8_t, 4> garbage = {0xFF, 0x00, 0xAB, 0xCD};
  auto result = Communication::deserializeState(garbage.data(), garbage.size());
  EXPECT_FALSE(result.has_value());
}

TEST(SerializationTest, DeserializeNullptrReturnsNullopt)
{
  auto result = Communication::deserializeState(nullptr, 0);
  EXPECT_FALSE(result.has_value());
}

TEST(SerializationTest, EmptyVectorsRoundtrip)
{
  State original;

  auto buffer = Communication::serializeState(original);
  auto result = Communication::deserializeState(buffer.data(), buffer.size());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->position.empty());
  EXPECT_TRUE(result->velocity.empty());
  EXPECT_TRUE(result->torque.empty());
}

// ============================================================
// 3. Bulk logging test (send/receive comparison)
// ============================================================

class CommunicationLogTest : public ::testing::Test
{
protected:
  static constexpr size_t dof = 6;
  static constexpr size_t num_cycles = 100;

  State randomState()
  {
    State s;
    s.position.resize(dof);
    s.velocity.resize(dof);
    s.torque.resize(dof);
    for(size_t i = 0; i < dof; ++i)
    {
      s.position[i] = dist_(rng_);
      s.velocity[i] = dist_(rng_);
      s.torque[i] = dist_(rng_);
    }
    return s;
  }

  Command randomCommand()
  {
    Command c;
    c.kp = dist_(rng_);
    c.kd = dist_(rng_);
    c.position.resize(dof);
    c.velocity.resize(dof);
    c.torque.resize(dof);
    for(size_t i = 0; i < dof; ++i)
    {
      c.position[i] = dist_(rng_);
      c.velocity[i] = dist_(rng_);
      c.torque[i] = dist_(rng_);
    }
    return c;
  }

private:
  std::mt19937 rng_{42};
  std::uniform_real_distribution<double> dist_{-10.0, 10.0};
};

TEST_F(CommunicationLogTest, AllStatesSurviveRoundtrip)
{
  std::vector<State> sent_log;
  std::vector<State> received_log;

  for(size_t i = 0; i < num_cycles; ++i)
  {
    State s = randomState();
    sent_log.push_back(s);

    auto buffer = Communication::serializeState(s);
    auto result = Communication::deserializeState(buffer.data(), buffer.size());

    ASSERT_TRUE(result.has_value()) << "Failed at cycle " << i;
    received_log.push_back(*result);
  }

  ASSERT_EQ(sent_log.size(), received_log.size());
  for(size_t i = 0; i < sent_log.size(); ++i)
  {
    EXPECT_EQ(sent_log[i].position, received_log[i].position) << "pos mismatch cycle " << i;
    EXPECT_EQ(sent_log[i].velocity, received_log[i].velocity) << "vel mismatch cycle " << i;
    EXPECT_EQ(sent_log[i].torque, received_log[i].torque) << "trq mismatch cycle " << i;
  }
}

TEST_F(CommunicationLogTest, AllCommandsSurviveRoundtrip)
{
  std::vector<Command> sent_log;
  std::vector<Command> received_log;

  for(size_t i = 0; i < num_cycles; ++i)
  {
    Command c = randomCommand();
    sent_log.push_back(c);

    auto buffer = Communication::serializeCommand(c);
    auto result = Communication::deserializeCommand(buffer.data(), buffer.size());

    ASSERT_TRUE(result.has_value()) << "Failed at cycle " << i;
    received_log.push_back(*result);
  }

  ASSERT_EQ(sent_log.size(), received_log.size());
  for(size_t i = 0; i < sent_log.size(); ++i)
  {
    EXPECT_DOUBLE_EQ(sent_log[i].kp, received_log[i].kp) << "kp mismatch cycle " << i;
    EXPECT_DOUBLE_EQ(sent_log[i].kd, received_log[i].kd) << "kd mismatch cycle " << i;
    EXPECT_EQ(sent_log[i].position, received_log[i].position) << "pos mismatch cycle " << i;
    EXPECT_EQ(sent_log[i].velocity, received_log[i].velocity) << "vel mismatch cycle " << i;
    EXPECT_EQ(sent_log[i].torque, received_log[i].torque) << "trq mismatch cycle " << i;
  }
}

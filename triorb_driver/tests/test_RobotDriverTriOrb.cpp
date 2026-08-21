#include <triorb_driver/RobotDriverTriOrb.h>

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace
{

class FakeTriOrbClient final : public triorb_driver::ITriOrbClient
{
public:
  void checkHealth() override
  {
    ++healthCalls;
  }

  void wakeup() override
  {
    ++wakeupCalls;
  }

  void sleep() override
  {
    ++sleepCalls;
  }

  void stop() override
  {
    ++stopCalls;
  }

  void setBodyVelocity(double vx, double vy, double wz) override
  {
    ++velocityCalls;
    lastVelocity = {vx, vy, wz};
  }

  triorb_driver::PlanarPose readPose() override
  {
    ++poseCalls;
    return nextPose;
  }

  int healthCalls = 0;
  int wakeupCalls = 0;
  int sleepCalls = 0;
  int stopCalls = 0;
  int velocityCalls = 0;
  int poseCalls = 0;

  std::vector<double> lastVelocity{0.0, 0.0, 0.0};

  triorb_driver::PlanarPose nextPose{1.0, 2.0, 0.5, true};
};

} // namespace

TEST(RobotDriverTriOrb, AcceptsThreeComponentVelocity)
{
  auto client = std::make_unique<FakeTriOrbClient>();

  triorb_driver::RobotDriverTriOrb driver(std::move(client), false, false);

  EXPECT_NO_THROW(driver.speedJ({0.1, -0.2, 0.3}));
}

TEST(RobotDriverTriOrb, RejectsWrongVelocitySize)
{
  auto client = std::make_unique<FakeTriOrbClient>();

  triorb_driver::RobotDriverTriOrb driver(std::move(client), false, false);

  EXPECT_THROW(driver.speedJ({0.1, 0.2}), std::invalid_argument);
}

TEST(RobotDriverTriOrb, RejectsNonFiniteVelocity)
{
  auto client = std::make_unique<FakeTriOrbClient>();

  triorb_driver::RobotDriverTriOrb driver(std::move(client), false, false);

  EXPECT_THROW(driver.speedJ({0.1, std::numeric_limits<double>::quiet_NaN(), 0.0}), std::invalid_argument);
}

TEST(RobotDriverTriOrb, SyncSendsCachedVelocityAndReadsPose)
{
  auto client = std::make_unique<FakeTriOrbClient>();

  auto * fake = client.get();

  triorb_driver::RobotDriverTriOrb driver(std::move(client), false, false);

  driver.speedJ({0.1, -0.2, 0.3});
  driver.sync();

  ASSERT_EQ(fake->velocityCalls, 1);

  EXPECT_DOUBLE_EQ(fake->lastVelocity[0], 0.1);
  EXPECT_DOUBLE_EQ(fake->lastVelocity[1], -0.2);
  EXPECT_DOUBLE_EQ(fake->lastVelocity[2], 0.3);

  EXPECT_EQ(driver.getActualQ(), (std::vector<double>{1.0, 2.0, 0.5}));
}

TEST(RobotDriverTriOrb, HasNoInventedVelocityFeedback)
{
  auto client = std::make_unique<FakeTriOrbClient>();

  triorb_driver::RobotDriverTriOrb driver(std::move(client), false, false);

  EXPECT_TRUE(driver.getActualQd().empty());
}

TEST(RobotDriverTriOrb, HasNoTorqueFeedback)
{
  auto client = std::make_unique<FakeTriOrbClient>();

  triorb_driver::RobotDriverTriOrb driver(std::move(client), false, false);

  EXPECT_TRUE(driver.getJointTorques().empty());
}

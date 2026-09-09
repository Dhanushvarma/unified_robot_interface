#include <triorb_driver/PlanarPositionControl.h>

#include <gtest/gtest.h>

#include <cmath>

namespace
{

triorb_driver::PlanarPositionControlConfig config()
{
  triorb_driver::PlanarPositionControlConfig result;

  result.translationGain = 1.0;
  result.rotationGain = 1.0;

  result.maximumLinearVelocity = 1.0;
  result.maximumAngularVelocity = 1.0;

  result.positionTolerance = 1e-6;
  result.angularTolerance = 1e-6;

  return result;
}

} // namespace

TEST(PlanarPositionControl, ProducesZeroVelocityAtTarget)
{
  const triorb_driver::PlanarPose measured{
      1.0,
      2.0,
      0.5,
  };

  const auto command = triorb_driver::computePlanarVelocity(measured, measured, config());

  EXPECT_DOUBLE_EQ(command.vx, 0.0);
  EXPECT_DOUBLE_EQ(command.vy, 0.0);
  EXPECT_DOUBLE_EQ(command.wz, 0.0);
}

TEST(PlanarPositionControl, ProducesAngularVelocityTowardTarget)
{
  const triorb_driver::PlanarPose measured{
      0.0,
      0.0,
      0.0,
  };

  const triorb_driver::PlanarPose target{
      0.0,
      0.0,
      -0.2,
  };

  const auto command = triorb_driver::computePlanarVelocity(measured, target, config());

  EXPECT_DOUBLE_EQ(command.vx, 0.0);
  EXPECT_DOUBLE_EQ(command.vy, 0.0);
  EXPECT_NEAR(command.wz, -0.2, 1e-12);
}

TEST(PlanarPositionControl, UsesShortestAngularError)
{
  constexpr double pi = 3.14159265358979323846;

  const triorb_driver::PlanarPose measured{
      0.0,
      0.0,
      pi - 0.1,
  };

  const triorb_driver::PlanarPose target{
      0.0,
      0.0,
      -pi + 0.1,
  };

  const auto command = triorb_driver::computePlanarVelocity(measured, target, config());

  EXPECT_NEAR(command.wz, 0.2, 1e-12);
}

TEST(PlanarPositionControl, ConvertsWorldErrorToBodyFrame)
{
  constexpr double pi = 3.14159265358979323846;

  const triorb_driver::PlanarPose measured{
      0.0,
      0.0,
      pi / 2.0,
  };

  const triorb_driver::PlanarPose target{
      1.0,
      0.0,
      pi / 2.0,
  };

  const auto command = triorb_driver::computePlanarVelocity(measured, target, config());

  EXPECT_NEAR(command.vx, 0.0, 1e-12);
  EXPECT_NEAR(command.vy, -1.0, 1e-12);
  EXPECT_DOUBLE_EQ(command.wz, 0.0);
}

TEST(PlanarPositionControl, LimitsLinearVelocityByVectorMagnitude)
{
  auto testConfig = config();

  testConfig.maximumLinearVelocity = 0.5;

  const triorb_driver::PlanarPose measured{
      0.0,
      0.0,
      0.0,
  };

  const triorb_driver::PlanarPose target{
      3.0,
      4.0,
      0.0,
  };

  const auto command = triorb_driver::computePlanarVelocity(measured, target, testConfig);

  EXPECT_NEAR(std::hypot(command.vx, command.vy), 0.5, 1e-12);
}

TEST(PlanarPositionControl, LimitsAngularVelocity)
{
  auto testConfig = config();

  testConfig.maximumAngularVelocity = 0.2;

  const triorb_driver::PlanarPose measured{
      0.0,
      0.0,
      0.0,
  };

  const triorb_driver::PlanarPose target{
      0.0,
      0.0,
      1.0,
  };

  const auto command = triorb_driver::computePlanarVelocity(measured, target, testConfig);

  EXPECT_DOUBLE_EQ(command.wz, 0.2);
}

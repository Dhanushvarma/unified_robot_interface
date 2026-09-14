#pragma once

#include <algorithm>
#include <cmath>

namespace triorb_driver
{

struct PlanarPose
{
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
  bool valid = false;
};

struct PlanarVelocity
{
  double vx = 0.0;
  double vy = 0.0;
  double wz = 0.0;
};

struct PlanarPositionControlConfig
{
  double translationGain = 0.0;
  double rotationGain = 0.0;

  double maximumLinearVelocity = 0.0;
  double maximumAngularVelocity = 0.0;

  double positionTolerance = 0.0;
  double angularTolerance = 0.0;
};

inline double wrapAngle(double angle)
{
  return std::atan2(std::sin(angle), std::cos(angle));
}

inline double clampSymmetric(double value, double maximumMagnitude)
{
  if(maximumMagnitude <= 0.0)
  {
    return 0.0;
  }

  return std::clamp(value, -maximumMagnitude, maximumMagnitude);
}

inline PlanarVelocity computePlanarVelocity(const PlanarPose & measured,
                                            const PlanarPose & target,
                                            const PlanarPositionControlConfig & config)
{
  const double errorXWorld = target.x - measured.x;

  const double errorYWorld = target.y - measured.y;

  const double angularError = wrapAngle(target.theta - measured.theta);

  const double cosTheta = std::cos(measured.theta);

  const double sinTheta = std::sin(measured.theta);

  const double errorXBody = cosTheta * errorXWorld + sinTheta * errorYWorld;

  const double errorYBody = -sinTheta * errorXWorld + cosTheta * errorYWorld;

  PlanarVelocity command;

  if(std::hypot(errorXWorld, errorYWorld) > config.positionTolerance)
  {
    command.vx = config.translationGain * errorXBody;

    command.vy = config.translationGain * errorYBody;
  }

  if(std::abs(angularError) > config.angularTolerance)
  {
    command.wz = config.rotationGain * angularError;
  }

  const double linearVelocity = std::hypot(command.vx, command.vy);

  if(linearVelocity > config.maximumLinearVelocity && linearVelocity > 0.0)
  {
    const double scale = config.maximumLinearVelocity / linearVelocity;

    command.vx *= scale;
    command.vy *= scale;
  }

  command.wz = clampSymmetric(command.wz, config.maximumAngularVelocity);

  return command;
}

} // namespace triorb_driver

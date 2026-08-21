#pragma once

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

} // namespace triorb_driver

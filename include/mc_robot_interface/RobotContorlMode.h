#pragma once

#include <mc_rbdyn/Robot.h>

#include <RBDyn/MultiBodyConfig.h>

#include <mc_robot_interface/RobotDriver.h>

namespace mc_rtc {
enum ControlMode { Position, Velocity, Torque };

template <typename cm> struct RobotControlMode {
  void control(RobotDriver &driver, const mc_rbdyn::Robot &robot,
               const rbd::MultiBodyConfig &mbc);
};
} // namespace mc_rtc

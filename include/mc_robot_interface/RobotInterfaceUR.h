#pragma once

#include <mc_robot_interface/RobotDriverRTDE.h>
#include <mc_robot_interface/RobotInterface.h>

namespace mc_robot
{

struct RobotInterfaceUR : public RobotInterface
{

public:
  RobotInterfaceUR(const std::string & name, const mc_rtc::Configuration & config);

  void init() override {}
  void reset() override {}
  void stop() override {}
  void updateSensors() override {}
  void updateControl() override {}
};

} // namespace mc_robot

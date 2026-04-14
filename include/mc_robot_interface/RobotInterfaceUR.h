#pragma once

#include <mc_robot_interface/RobotDriverRTDE.h>
#include <mc_robot_interface/RobotInterface.h>

namespace mc_rtde
{

struct RobotInterfaceUR : public mc_robot::RobotInterface
{

public:
  RobotInterfaceUR(const std::string & name, const mc_rtc::Configuration & config, uint8_t buffer_size = 6);

  void init() override {}
  void reset() override {}
  void stop() override {}
  void updateSensors() override {}
  void updateControl() override {}
};

} // namespace mc_rtde

#pragma once

#include <mc_rtc/Configuration.h>
#include <mc_robot_interface/RobotInterfaceBase.h>
#include <robot_comm/CommunicationFactory.h>

#include <memory>
#include <string>

namespace mc_robot
{

struct RobotInterfaceFactory
{
public:
  void checkCompatibility();

  static std::unique_ptr<RobotInterfaceBase> makeInterface(const std::string & name,
                                                           const mc_rtc::Configuration & config,
                                                           const uint8_t & buffer_size = 0);
};

} // namespace mc_robot

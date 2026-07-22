#pragma once

#include <mc_communication/CommunicationFactory.h>
#include <mc_rtc/Configuration.h>
#include <mc_robot_interface/RobotInterfaceBase.h>
#include <string>

namespace mc_robot
{

struct RobotInterfaceFactory
{
public:
  void checkCompatibility();

  static std::unique_ptr<RobotInterfaceBase> makeInterface(const std::string & name,
                                                           const mc_rtc::Configuration & config);
};

} // namespace mc_robot

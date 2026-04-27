#pragma once

#include <mc_communication/CommunicationFactory.h>
#include <mc_rtc/Configuration.h>
#include <mc_robot_interface/RobotInterface.h>
#include <string>

namespace mc_robot
{

struct RobotInterfaceFactory
{
public:
  void checkCompatibility();

  static std::unique_ptr<RobotInterface> makeInterface(const std::string & name,
                                                       const mc_rtc::Configuration & config,
                                                       const uint8_t & buffer_size = 0);
};

} // namespace mc_robot

#pragma once

#include <mc_rtc/Configuration.h>
#include <robot_comm/CommunicationFactory.h>
#include <robot_interface/RobotInterfaceBase.h>

#include <memory>
#include <string>

namespace mc_robot
{

/// Creates the manager-side proxy of a robot.
struct RobotInterfaceFactory
{
public:
  void checkCompatibility();

  /// Create the proxy of robot `name` from its configuration entry.
  static std::unique_ptr<RobotInterfaceBase> makeInterface(const std::string & name,
                                                           const mc_rtc::Configuration & config,
                                                           const uint8_t & buffer_size = 0);
};

} // namespace mc_robot

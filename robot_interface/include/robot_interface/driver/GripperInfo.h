#pragma once

#include <string>
#include <vector>

namespace mc_robot_interface
{

/// A gripper declared in the robot's RobotModule, passed to the driver's
/// create() so drivers learn about grippers without depending on mc_rtc.
struct GripperInfo
{
  /// Gripper name.
  std::string name;
  /// Names of the joints it actuates.
  std::vector<std::string> joints;
};

} // namespace mc_robot_interface

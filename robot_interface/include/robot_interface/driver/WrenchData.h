#pragma once

#include <array>
#include <map>
#include <string>

namespace robot_interface
{

/// One force/torque sensor reading, in the sensor frame, as reported by
/// RobotDriver::getForceSensors().
struct WrenchData
{
  /// [N]
  std::array<double, 3> force = {0.0, 0.0, 0.0};
  /// [Nm]
  std::array<double, 3> torque = {0.0, 0.0, 0.0};
};

/// Force sensor name (as in the RobotModule, e.g. "EEForceSensor") -> latest reading.
using WrenchMap = std::map<std::string, WrenchData>;

} // namespace robot_interface

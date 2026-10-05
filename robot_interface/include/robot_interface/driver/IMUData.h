#pragma once

#include <array>
#include <map>
#include <string>

namespace robot_interface
{

/// One IMU reading, as reported by RobotDriver::getIMUs().
struct IMUData
{
  /// Quaternion (w, x, y, z). Identity if the sensor has no orientation.
  std::array<double, 4> orientation = {1.0, 0.0, 0.0, 0.0};
  /// [rad/s]
  std::array<double, 3> angularVelocity = {0.0, 0.0, 0.0};
  /// [m/s²]
  std::array<double, 3> linearAcceleration = {0.0, 0.0, 0.0};
};

/// Body sensor name (as in the RobotModule, e.g. "Accelerometer") -> latest reading.
using IMUMap = std::map<std::string, IMUData>;

} // namespace robot_interface

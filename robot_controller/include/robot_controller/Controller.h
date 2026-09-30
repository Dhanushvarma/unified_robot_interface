#pragma once

#include <array>
#include <string>
#include <vector>

namespace robot_controller
{

/// Type of command a robot receives (`controller.mode`).
enum class ControlMode
{
  POSITION,
  VELOCITY,
  TORQUE
};

/// Control framework run by `uri manager`, loaded as a backend plugin.
///
/// Implement it to support a controller other than mc_rtc (see
/// docs/RobotManager.md, "Controller backend"). Robots are identified by
/// their name under `Robots:`, and joint vectors follow each robot's
/// reference joint order.
class Controller
{
public:
  virtual ~Controller() = default;

  /// Run one control step.
  virtual void run() = 0;
  /// Whether the controller is running.
  virtual bool isRunning() const = 0;
  /// Start running.
  virtual void start() = 0;
  /// Stop running.
  virtual void stop() = 0;

  /// Control period [s].
  virtual double timeStep() const = 0;

  /// Set the measured joint positions [rad] of `robot`.
  virtual void setEncoderValues(const std::string & robot, const std::vector<double> & values) = 0;

  /// Set the measured joint velocities [rad/s] of `robot`.
  virtual void setEncoderVelocities(const std::string & robot, const std::vector<double> & values) = 0;

  /// Set the measured joint torques [Nm] of `robot`.
  virtual void setJointTorques(const std::string & robot, const std::vector<double> & values) = 0;

  /// Initialize `robotName` from its first joint positions [rad]. Called once
  /// per robot, when its first state arrives.
  virtual void initializeRobot(const std::string & robotName, const std::vector<double> & encoderValues) = 0;

  /// Called once after all robots are known, before the control loop starts.
  virtual void initializeRobots() = 0;

  /// Set an IMU reading of `robot`: orientation quaternion (w, x, y, z),
  /// angular velocity [rad/s] and linear acceleration [m/s²].
  virtual void setBodySensor(const std::string & robot,
                             const std::string & sensorName,
                             const std::array<double, 4> & orientation,
                             const std::array<double, 3> & angularVelocity,
                             const std::array<double, 3> & linearAcceleration) = 0;

  /// Set a force/torque reading of `robot`, in the sensor frame: force [N]
  /// and torque [Nm].
  virtual void setForceSensor(const std::string & robot,
                              const std::string & sensorName,
                              const std::array<double, 3> & force,
                              const std::array<double, 3> & torque) = 0;

  /// Latest command for `robot` in the given mode.
  virtual std::vector<double> command(const std::string & robot, ControlMode mode) const = 0;
};

} // namespace robot_controller

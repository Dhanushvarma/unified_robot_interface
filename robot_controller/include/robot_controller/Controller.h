#pragma once

#include <string>
#include <vector>

namespace robot_controller
{

enum class ControlMode
{
  POSITION,
  VELOCITY,
  TORQUE
};

class Controller
{
public:
  virtual ~Controller() = default;

  virtual void run() = 0;
  virtual bool isRunning() const = 0;
  virtual void start() = 0;
  virtual void stop() = 0;

  virtual double timeStep() const = 0;

  virtual void setEncoderValues(const std::string & robot, const std::vector<double> & values) = 0;

  virtual void setEncoderVelocities(const std::string & robot, const std::vector<double> & values) = 0;

  virtual void setJointTorques(const std::string & robot, const std::vector<double> & values) = 0;

  virtual void initializeRobot(const std::string & robotName, const std::vector<double> & encoderValues) = 0;

  virtual void initializeRobots() = 0;

  virtual std::vector<double> command(const std::string & robot, ControlMode mode) const = 0;
};

} // namespace robot_controller

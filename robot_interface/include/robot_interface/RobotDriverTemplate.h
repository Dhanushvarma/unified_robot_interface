#pragma once

#include <robot_interface/driver/api.h>

namespace mc_robot_interface
{
struct MC_ROBOT_DRIVER_DLLAPI RobotDriver
{
  virtual ~RobotDriver() = default;
  virtual void sync() = 0;
  virtual void setDataRead() {}
  virtual std::vector<double> getActualQ() = 0;
  virtual std::vector<double> getJointTorques() = 0;
  virtual void servoJ(const std::vector<double> & q) = 0;
  virtual void speedJ(const std::vector<double> & alpha) = 0;
};

typedef std::shared_ptr<RobotDriver> RobotDriverPtr;

} // namespace mc_robot_interface

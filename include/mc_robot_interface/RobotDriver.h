#pragma once

#include <string>
#include <vector>

namespace mc_rtc {
struct RobotDriver {

public:
  RobotDriver() = default;
  virtual ~RobotDriver() = default;

  /**
   * @brief Wait for robot to be initialized
   *
   */
  virtual void sync() = 0;

  virtual void setDataRead() {}

  /**
   * @brief Get Robot Actual Q
   *
   * @return std::vector<double>
   */
  virtual std::vector<double> getActualQ() = 0;

  /**
   * @brief Get Robot Joint Torques
   *
   * @return std::vector<double>
   */
  virtual std::vector<double> getJointTorques() = 0;

  /**
   * @brief Send joint angle commands to robot
   *
   * @param q
   */
  virtual void servoJ(const std::vector<double> &q) = 0;

  /**
   * @brief Send joint velocities commands to robot
   *
   * @param alpha
   */
  virtual void speedJ(const std::vector<double> &alpha) = 0;

private:
  std::string name_ = "default";
};
} // namespace mc_rtc

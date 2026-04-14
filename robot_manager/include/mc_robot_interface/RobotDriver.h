#pragma once

#include <mc_rtc/logging.h>
#include <string>
#include <vector>

// TODO: should this be in mc_rtc
// should we change it to mc_driver
namespace mc_rtc
{

class RobotDriver
{
public:
  RobotDriver(const std::string & ip, const uint16_t & port = 0)
  {
    mc_rtc::log::info("[driver] Starting driver with ip {} port {}", ip, port);
  };

  virtual ~RobotDriver() = default;
  RobotDriver(const RobotDriver &) = delete;
  RobotDriver & operator=(const RobotDriver &) = delete;
  RobotDriver(RobotDriver &&) = delete;
  RobotDriver & operator=(RobotDriver &&) = delete;

  /**
   * @brief Wait for robot to be initialized
   *
   */
  virtual void sync() {}

  virtual void setDataRead() {}

  /**
   * @brief Get robot joint positions
   *
   * @return std::vector<double>
   */
  virtual std::vector<double> getPosition() = 0;

  /**
   * @brief Get robot joint velocities
   *
   * @return std::vector<double>
   */
  virtual std::vector<double> getVelocity() = 0;

  /**
   * @brief Get robot joint torques
   *
   * @return std::vector<double>
   */
  virtual std::vector<double> getTorque() = 0;

  /**
   * @brief Send joint angle commands to robot
   *
   * @param command
   */
  virtual void setPosition(const std::vector<double> & command) = 0;

  /**
   * @brief Send joint velocity commands to robot
   *
   * @param command
   */
  virtual void setVelocity(const std::vector<double> & command) = 0;

  /**
   * @brief Send joint torque commands to robot
   *
   * @param command
   */
  virtual void setTorque(const std::vector<double> & command) = 0;

  /**
   * @brief Set robot in freedrive mode, allow the robot to be moved around by hand
   *
   * @param enable
   * @return true when the robot is in freedrive mode, false otherwise.
   */
  virtual bool freeDrive(bool enable)
  {
    return false;
  }
};

} // namespace mc_rtc

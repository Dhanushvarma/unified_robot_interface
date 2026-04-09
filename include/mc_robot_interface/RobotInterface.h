#pragma once

#include <mc_rtc/Configuration.h>

#include <mc_robot_interface/RobotDriver.h>

namespace mc_robot
{

struct RobotInterface
{

public:
  RobotInterface() = default;
  virtual ~RobotInterface() = default;
  RobotInterface(const RobotInterface &) = delete;
  RobotInterface & operator=(const RobotInterface &) = delete;
  RobotInterface(RobotInterface &&) = delete;
  RobotInterface & operator=(RobotInterface &&) = delete;

  RobotInterface(const mc_rtc::Configuration & config);

  virtual void init() = 0;
  virtual void reset() = 0;
  virtual void stop() = 0;

  virtual void updateSensors() = 0;
  virtual void updateControl() = 0;

  template<typename cm>
  void control();

  void loadConfig(const mc_rtc::Configuration & config);

private:
  mutable std::mutex update_sensor_mtx_{};
  mutable std::mutex update_control_mtx_{};

  mc_rtc::Configuration config_{};
  std::unique_ptr<mc_rtc::RobotDriver> driver_{};
  double dt_{};

  /**
   * @brief Method in charge of robot sensors and commands update
   *
   */
  void controlThread();
};

} // namespace mc_robot

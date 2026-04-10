#pragma once

#include <mc_rtc/Configuration.h>

#include <mc_robot_interface/RobotDriver.h>

namespace mc_robot
{

struct RobotInterface
{

public:
  RobotInterface(const std::string & name, const mc_rtc::Configuration & config)
  : name_(name), config_(config), ip_(config_("network")("ip"))
  {
    mc_rtc::log::info("name_ {}", name_);
    mc_rtc::log::info("ip_ {}", ip_);
    mc_rtc::log::info(config.dump(true, true));
  };

  virtual ~RobotInterface() = default;
  RobotInterface(const RobotInterface &) = delete;
  RobotInterface & operator=(const RobotInterface &) = delete;
  RobotInterface(RobotInterface &&) = delete;
  RobotInterface & operator=(RobotInterface &&) = delete;

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

  const std::string name_{};
  const mc_rtc::Configuration config_{};
  const std::string ip_{};

  const std::unique_ptr<mc_rtc::RobotDriver> driver_{};
  const double dt_{};

  /**
   * @brief Method in charge of robot sensors and commands update
   *
   */
  void controlThread();
};

} // namespace mc_robot

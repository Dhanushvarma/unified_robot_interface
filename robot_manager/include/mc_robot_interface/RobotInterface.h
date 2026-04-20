#pragma once

#include <mc_rtc/Configuration.h>

#include <mc_network_interface/NetworkInterface.h>
#include <mc_network_interface/NetworkInterfaceFactory.h>
#include <mc_robot_interface/RobotDriver.h>

namespace mc_robot
{

class RobotInterface
{
public:
  RobotInterface() {};

  RobotInterface(std::string name, mc_rtc::Configuration config, uint8_t buffer_size)
  : name_(std::move(name)), config_(std::move(config)), dt_(config_("controller")("time_step")),
    buffer_size_(buffer_size) {};
  // mc_rtc::log::info("name_ {}", name_);
  // mc_rtc::log::info("dt_ {}", dt_);
  // mc_rtc::log::info("protocol_ {}", protocol_);
  // mc_rtc::log::info("ip_ {}", ip_);
  // mc_rtc::log::info("port_ {}", port_);
  // mc_rtc::log::info("buffer_size_ {}", buffer_size_);

  // mc_rtc::log::info("config_ {}", config_.dump(true, true));

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

  // TODO: use loadConfig instead of constructor to process
  void loadConfig(const mc_rtc::Configuration & config);

protected:
  [[nodiscard]] const std::string & name() const
  {
    return name_;
  }
  [[nodiscard]] const mc_rtc::Configuration & config() const
  {
    return config_;
  }
  [[nodiscard]] const double dt() const
  {
    return dt_;
  }
  [[nodiscard]] const uint8_t buffer_size() const
  {
    return buffer_size_;
  }
  [[nodiscard]] mc_rtc::RobotDriver & driver()
  {
    return *driver_;
  }
  [[nodiscard]] mc_network::NetworkInterface & network()
  {
    return *network_;
  }

  // void setDriver(std::unique_ptr<mc_rtc::RobotDriver> driver)
  // {
  //   driver_ = std::move(driver);
  // }

private:
  mutable std::mutex update_sensor_mtx_{};
  mutable std::mutex update_control_mtx_{};

  const std::string name_{};
  const mc_rtc::Configuration config_{};
  const double dt_{};
  const uint8_t buffer_size_{};
  // const uint16_t port_{};

  std::vector<double> state_{};
  std::vector<double> command_{};

  // TODO: control mode ?
  std::unique_ptr<mc_rtc::RobotDriver> driver_{};
  std::unique_ptr<mc_network::NetworkInterface> network_{};

  /**
   * @brief Method in charge of robot sensors and commands update
   *
   */
  void controlThread();
};

} // namespace mc_robot

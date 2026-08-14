#pragma once

#include <robot_comm/Communication.h>
#include <robot_comm/CommunicationFactory.h>
#include <robot_controller/Controller.h>

#include <mc_rtc/Configuration.h>

#include <condition_variable>

namespace mc_robot
{

enum ControlMode
{
  POSITION = 0,
  VELOCITY,
  TORQUE
};

class RobotInterfaceBase
{
public:
  RobotInterfaceBase() = default;

  RobotInterfaceBase(std::string name, mc_rtc::Configuration config, uint8_t buffer_size)
  : name_(std::move(name)), config_(std::move(config)), dt_(config_("controller")("time_step")),
    buffer_size_(buffer_size) {};

  virtual ~RobotInterfaceBase() = default;
  RobotInterfaceBase(const RobotInterfaceBase &) = delete;
  RobotInterfaceBase & operator=(const RobotInterfaceBase &) = delete;
  RobotInterfaceBase(RobotInterfaceBase &&) = delete;
  RobotInterfaceBase & operator=(RobotInterfaceBase &&) = delete;

  virtual void init() = 0;
  virtual void reset() = 0;
  virtual void stop() = 0;

  virtual void updateSensors(robot_controller::Controller & gc) = 0;
  virtual void updateControl(robot_controller::Controller & gc) = 0;

  // Returns true once the interface has fed initial sensor values into mc_rtc
  // and called gc.init(). mainThread uses this to gate the first controller.run().
  [[nodiscard]] virtual bool isInitialized() const
  {
    return true;
  }

  /**
   * @brief Method in charge of robot sensors and commands update
   *
   */
  void controlThread(robot_controller::Controller & controller,
                     std::mutex & startM,
                     std::condition_variable & start_cv_,
                     bool & start) {
    // Not include `bool & running` like usual.
    // The value can be extract or change using methods defined in robot_controller::Controller
    // - isRunning()
    // - start()
    // - stop()
  };

  template<typename cm>
  void control();

  // TODO: use loadConfig instead of constructor to process
  void loadConfig(const mc_rtc::Configuration & config);

  void setCommunication(std::unique_ptr<robot_comm::Communication> communication)
  {
    communication_ = std::move(communication);
  }

  [[nodiscard]] const std::string & name() const
  {
    return name_;
  }
  [[nodiscard]] const mc_rtc::Configuration & config() const
  {
    return config_;
  }
  [[nodiscard]] double dt() const
  {
    return dt_;
  }
  [[nodiscard]] uint8_t bufferSize() const
  {
    return buffer_size_;
  }

  [[nodiscard]] robot_comm::Communication & communication()
  {
    return *communication_;
  }

private:
  mutable std::mutex update_sensor_mtx_{};
  mutable std::mutex update_control_mtx_{};

  const std::string name_{};
  const mc_rtc::Configuration config_{};
  const double dt_{};
  const uint8_t buffer_size_{};

  std::vector<double> state_{};
  std::vector<double> command_{};

  std::unique_ptr<robot_comm::Communication> communication_{};

protected:
  ControlMode control_mode_ = POSITION;
};

} // namespace mc_robot

#pragma once

#include <robot_interface/RobotDriverTemplate.h>
#include <triorb_driver/PlanarPositionControl.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace triorb_driver
{

class RobotDriverTriOrb final : public mc_robot_interface::RobotDriver
{
public:
  RobotDriverTriOrb(const std::string & device, uint16_t);

  ~RobotDriverTriOrb() override;

  void sync() override;

  std::vector<double> getActualQ() override;
  std::vector<double> getActualQd() override;
  std::vector<double> getJointTorques() override;

  void servoJ(const std::vector<double> & q) override;

  void speedJ(const std::vector<double> & velocity) override;

  void tauJ(const std::vector<double> & torque) override;

private:
  struct Command
  {
    uint16_t code = 0;
    std::vector<uint8_t> payload;
  };

  enum class ControlMode
  {
    Velocity,
    Position,
  };

  ControlMode controlMode_ = ControlMode::Velocity;

  PlanarPose targetPose_;

  PlanarPositionControlConfig positionControlConfig_;

private:
  void connect();
  void disconnect() noexcept;

  bool openPort();
  void closePort() noexcept;
  void flushIo();

  bool writeAll(const uint8_t * data, std::size_t size);

  bool readFrame(std::size_t expectedLength, std::vector<uint8_t> & response);

  std::vector<uint8_t> buildFrame(const std::vector<Command> & commands) const;

  bool transact(const std::vector<Command> & commands, std::vector<uint8_t> & response);

  bool sendWakeup();
  bool sendSleep();
  bool sendResetOrigin();
  bool sendVelocity(double vx, double vy, double wz);

  bool sendStop();
  bool readOdometry();

  void validateVelocity(const std::vector<double> & velocity) const;

  void paceCycle();

private:
  using Clock = std::chrono::steady_clock;

  static constexpr unsigned int defaultBaudrate_ = 115200;

  static constexpr double readTimeout_ = 0.1;

  static constexpr double frameTimeout_ = 0.5;

  static constexpr uint32_t watchdogMs_ = 100;

  static constexpr auto wakeupSettle_ = std::chrono::seconds{1};

  static constexpr auto cyclePeriod_ = std::chrono::milliseconds{20};

  std::string device_;
  int fd_ = -1;

  bool connected_ = false;
  bool transactionOk_ = false;

  PlanarVelocity command_;
  PlanarPose pose_;

  Clock::time_point nextCycle_;
  bool timerInitialized_ = false;
};

} // namespace triorb_driver

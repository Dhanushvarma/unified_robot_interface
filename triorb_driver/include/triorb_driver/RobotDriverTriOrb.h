#pragma once

#include <triorb_driver/TriOrbClient.h>
#include <triorb_driver/Types.h>

#include <robot_interface/RobotDriverTemplate.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace triorb_driver
{

class RobotDriverTriOrb final : public mc_robot_interface::RobotDriver
{
public:
  RobotDriverTriOrb(const std::string & ip, uint16_t port);

  RobotDriverTriOrb(std::unique_ptr<ITriOrbClient> client, bool wakeupOnConnect, bool sleepOnDisconnect);

  ~RobotDriverTriOrb() override;

  void sync() override;

  std::vector<double> getActualQ() override;
  std::vector<double> getActualQd() override;
  std::vector<double> getJointTorques() override;

  void servoJ(const std::vector<double> & q) override;

  void speedJ(const std::vector<double> & velocity) override;

  void tauJ(const std::vector<double> & torque) override;

  bool freeDrive(bool enable);

private:
  void connect();
  void disconnect() noexcept;

  void validateVelocity(const std::vector<double> & velocity) const;

  void paceCycle();

private:
  using Clock = std::chrono::steady_clock;

  static constexpr uint16_t defaultHttpPort_ = 8080;

  static constexpr auto requestTimeout_ = std::chrono::milliseconds{100};

  static constexpr auto wakeupSettle_ = std::chrono::seconds{1};

  static constexpr auto cyclePeriod_ = std::chrono::milliseconds{20};

  std::unique_ptr<ITriOrbClient> client_;

  bool wakeupOnConnect_ = true;
  bool sleepOnDisconnect_ = true;
  bool connected_ = false;

  PlanarVelocity command_;
  PlanarPose pose_;

  Clock::time_point nextCycle_;
  bool timerInitialized_ = false;
};

} // namespace triorb_driver

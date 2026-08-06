#pragma once

#include <robot_interface/RobotDriverTemplate.h>

#include <memory>
#include <string>
#include <vector>

#include <ur_client_library/rtde/data_package.h>
#include <ur_client_library/ur/ur_driver.h>

namespace rtde_driver
{

class RobotDriverRTDE : public mc_robot_interface::RobotDriver
{
public:
  RobotDriverRTDE(const std::string & ip, uint16_t port = 0);

  ~RobotDriverRTDE() override;

  void sync() override;

  void setDataRead() override {}

  std::vector<double> getActualQ() override;
  std::vector<double> getActualQd() override;
  std::vector<double> getJointTorques() override;

  void servoJ(const std::vector<double> & q) override;
  void speedJ(const std::vector<double> & alpha) override;
  void tauJ(const std::vector<double> & tau) override;

  // Enable / disable freedrive mode.
  bool freeDrive(bool enable);

private:
  std::string ip_;
  std::vector<std::string> output_recipe_;
  std::unique_ptr<urcl::UrDriver> driver_;
  std::unique_ptr<urcl::rtde_interface::DataPackage> data_pkg_;
};

} // namespace rtde_driver

// ── robot_interface plugin symbols ────────────────────────────────────────────
#include <robot_interface/driver/api.h>

extern "C"
{
  MC_ROBOT_DRIVER_DLLAPI void MC_RTC_ROBOT_DRIVER(std::vector<std::string> & classes);

  MC_ROBOT_DRIVER_DLLAPI mc_robot_interface::RobotDriver * create(const std::string & name,
                                                                  const std::string & ip,
                                                                  const uint16_t & port);

  MC_ROBOT_DRIVER_DLLAPI void destroy(mc_robot_interface::RobotDriver * ptr);
}

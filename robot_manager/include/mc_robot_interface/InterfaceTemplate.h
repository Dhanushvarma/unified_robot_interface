#pragma once

#include <mc_robot_interface/RobotInterfaceBase.h>

#include <atomic>
#include <cstdint>

namespace mc_interface_template
{

void run(void * data, const std::atomic<bool> & interrupt);
void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt);

class InterfaceTemplate : public mc_robot::RobotInterfaceBase
{
public:
  InterfaceTemplate(const std::atomic<bool> & interrupt);

  void init() override {}
  void reset() override {}
  void stop() override {}
  void updateSensors() override;
  void updateControl() override;

  void setDriver(std::unique_ptr<mc_rtc::RobotDriver> driver)
  {
    driver_ = std::move(driver);
  }

  [[nodiscard]] mc_rtc::RobotDriver & driver()
  {
    return *driver_;
  }

private:
  std::unique_ptr<mc_rtc::RobotDriver> driver_{};
};

} // namespace mc_interface_template

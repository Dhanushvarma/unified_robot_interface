#pragma once

#include <mc_robot_interface/RobotInterfaceBase.h>

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>

namespace mc_interface_template
{

class FMInterfaceTemplate : public mc_robot::RobotInterfaceBase
{
public:
  FMInterfaceTemplate(const std::string & name, const mc_rtc::Configuration & config, uint8_t buffer_size = 6);

  void init() override {}
  void reset() override {}
  void stop() override {}
  void updateSensors(robot_controller::Controller & gc) override;
  void updateControl(robot_controller::Controller & gc) override;

  [[nodiscard]] bool isInitialized() const override
  {
    return gc_initialized_;
  }

private:
  bool gc_initialized_ = false;
};

} // namespace mc_interface_template

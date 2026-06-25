#pragma once

#include <mc_robot_interface/RobotInterfaceBase.h>

#include <atomic>
#include <cstdint>

namespace mc_interface_template
{

class FMInterfaceTemplate : public mc_robot::RobotInterfaceBase
{
public:
  FMInterfaceTemplate(const std::string & name, const mc_rtc::Configuration & config, uint8_t buffer_size = 6);

  void init() override {}
  void reset() override {}
  void stop() override {}
  void updateSensors() override;
  void updateControl() override;
};

} // namespace mc_interface_template

#pragma once

#include <mc_robot_interface/RobotInterface.h>

namespace mc_interface_template
{

class InterfaceTemplate : public mc_robot::RobotInterface
{
public:
  InterfaceTemplate(const std::string & name, const mc_rtc::Configuration & config, const uint8_t & buffer_size = 7);

  void init() override {}
  void reset() override {}
  void stop() override {}
  void updateSensors() override {}
  void updateControl() override {}

private:
  /* data */
};

} // namespace mc_interface_template

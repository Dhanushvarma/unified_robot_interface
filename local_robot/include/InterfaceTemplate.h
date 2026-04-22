#pragma once

#include <mc_robot_interface/RobotInterface.h>

#include <atomic>
#include <cstdint>

namespace mc_interface_template
{

void run(void * data, const std::atomic<bool> & interrupt);
void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt);

class InterfaceTemplate : public mc_robot::RobotInterface
{
public:
  InterfaceTemplate(const std::atomic<bool> & interrupt);
  InterfaceTemplate(const std::string & name, const mc_rtc::Configuration & config, uint8_t buffer_size = 6);

  void init() override {}
  void reset() override {}
  void stop() override {}
  void updateSensors() override {}
  void updateControl() override {}
};

} // namespace mc_interface_template

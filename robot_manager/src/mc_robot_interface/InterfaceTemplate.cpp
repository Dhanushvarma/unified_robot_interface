#include <mc_robot_interface/InterfaceTemplate.h>

namespace mc_interface_template
{

InterfaceTemplate::InterfaceTemplate(const std::string & name,
                                     const mc_rtc::Configuration & config,
                                     const uint8_t & buffer_size)
: RobotInterface(name, config, (buffer_size == 0) ? 7 : buffer_size)
{
  mc_rtc::log::success("InterfaceTemplate start");

  // TODO: pick driver
};

} // namespace mc_interface_template

#include <mc_rtc/logging.h>
#include <mc_robot_interface/InterfaceTemplate.h>
#include <mc_robot_interface/RobotInterfaceFactory.h>

namespace mc_robot
{

std::unique_ptr<RobotInterface> RobotInterfaceFactory::makeInterface(const std::string & name,
                                                                     const mc_rtc::Configuration & config,
                                                                     const uint8_t & buffer_size)
{
  mc_rtc::log::success("makeInterface start");
  const std::string module = config("module");

  if(module == "interface_template")
  {
    return std::make_unique<mc_interface_template::InterfaceTemplate>(name, config, buffer_size);
  }

  mc_rtc::log::error("Robot module {} is not supported", module);
  return nullptr;
}

} // namespace mc_robot

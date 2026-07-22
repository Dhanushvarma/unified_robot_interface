#include <mc_robot_interface/FMInterfaceTemplate.h>
#include <mc_robot_interface/RobotInterfaceFactory.h>

#include <mc_rtc/logging.h>

namespace mc_robot
{

std::unique_ptr<RobotInterfaceBase> RobotInterfaceFactory::makeInterface(const std::string & name,
                                                                         const mc_rtc::Configuration & config)
{
  mc_rtc::log::success("makeInterface start");
  const std::string module = config("module");

  if(module == "interface_template")
  {
    return std::make_unique<mc_interface_template::FMInterfaceTemplate>(name, config);
  }

  mc_rtc::log::error("Robot module {} is not supported", module);
  return nullptr;
}

} // namespace mc_robot

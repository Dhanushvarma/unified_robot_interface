#include <mc_rtc/logging.h>
#include <mc_robot_interface/InterfaceTemplate.h>
#include <mc_robot_interface/RobotInterfaceFactory.h>

namespace mc_robot
{

void RobotInterfaceFactory::addRobotInterface(const std::string & name, const mc_rtc::Configuration & config)
{
  mc_rtc::log::success("interface addRobotInterface start");

  const std::string module = config("module");

  if(module == "local_robot")
  {
    mc_rtc::log::info("interface addRobotInterface module {}", module);

    /* Initialize network */
    // Ensure network compatibility before additional initialization
    // TODO: potentially this coupled behaviour need to change
  }
  else
  {
    mc_rtc::log::warning("Robot module {} is not supported", module);
  }

  mc_rtc::log::info("interface addRobotInterface done");
}

void RobotInterfaceFactory::checkCompatibility() {};

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

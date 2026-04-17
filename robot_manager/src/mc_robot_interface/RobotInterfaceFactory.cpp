#include <mc_rtc/logging.h>
#include <mc_robot_interface/RobotInterfaceFactory.h>
#include <mc_robot_interface/RobotInterfaceUR.h>

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

} // namespace mc_robot

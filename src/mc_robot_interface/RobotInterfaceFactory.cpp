#include <mc_rtc/logging.h>
#include <mc_robot_interface/RobotInterfaceFactory.h>
#include <mc_robot_interface/RobotInterfaceUR.h>

namespace mc_robot
{

void RobotInterfaceFactory::addRobotInterface(const std::string & name, const mc_rtc::Configuration & config)
{
  const std::string module = config("module");

  if(module == "ur5e")
  {
    auto new_robot = std::make_unique<RobotInterfaceUR>(name, config);
    auto [it, success] = robots_interfaces_.try_emplace(name, std::move(new_robot));
    if(!success)
    {
      mc_rtc::log::warning("Robot {} already exists", name);
    }
  }
  else
  {
    mc_rtc::log::warning("Robot module {} is not supported", module);
  }
}

} // namespace mc_robot

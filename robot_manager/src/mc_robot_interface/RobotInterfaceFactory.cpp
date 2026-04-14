#include <mc_rtc/logging.h>
#include <mc_robot_interface/RobotInterfaceFactory.h>
#include <mc_robot_interface/RobotInterfaceUR.h>

namespace mc_robot
{

void RobotInterfaceFactory::addRobotInterface(const std::string & name,
                                              const mc_rtc::Configuration & config,
                                              mc_network::NetworkInterfaceFactory & network_interface_factory)
{
  mc_rtc::log::success("interface addRobotInterface start");

  const std::string module = config("module");

  if(module == "ur5e")
  {
    mc_rtc::log::info("interface addRobotInterface module {}", module);

    /* Initialize network */
    // Ensure network compatibility before additional initialization
    // TODO: potentially this coupled behaviour need to change
    if(network_interface_factory.addNetworkInterface(name, config))
    {
      auto new_robot = std::make_unique<mc_rtde::RobotInterfaceUR>(name, config);
      auto [it, success] = robot_interfaces_.try_emplace(name, std::move(new_robot));
      if(!success)
      {
        mc_rtc::log::warning("Robot {} already exists", name);
      }
    }
  }
  else
  {
    mc_rtc::log::warning("Robot module {} is not supported", module);
  }

  mc_rtc::log::info("interface addRobotInterface done");
}

} // namespace mc_robot

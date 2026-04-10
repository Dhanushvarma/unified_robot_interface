#include <mc_robot_interface/RobotInterfaceUR.h>

namespace mc_robot
{

RobotInterfaceUR::RobotInterfaceUR(const std::string & name, const mc_rtc::Configuration & config)
: RobotInterface(name, config)
{
  const std::string driver_name{config_("controller")("driver", std::string("ur_rtde"))};
  if(driver_name == "ur_rtde")
  {
    driver_ = std::make_unique<mc_rtde::RobotDriverRTDE>(ip_);
  }
  else
  {
    mc_rtc::log::error("[interface_ur] Driver {} is not available for robot {}", driver_name, name_);
  }
};

} // namespace mc_robot

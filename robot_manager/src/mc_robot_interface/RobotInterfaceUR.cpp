#include <mc_robot_interface/RobotInterfaceUR.h>

namespace mc_rtde
{

RobotInterfaceUR::RobotInterfaceUR(const std::string & name, const mc_rtc::Configuration & config, uint8_t buffer_size)
: RobotInterface(name, config, buffer_size)
{
  const std::string driver_name{RobotInterface::config()("controller")("driver", std::string("ur_rtde"))};
  if(driver_name == "ur_rtde")
  {
    RobotInterface::setDriver(std::make_unique<mc_rtde::RobotDriverRTDE>(RobotInterface::ip()));
  }
  else
  {
    mc_rtc::log::error("[interface_ur] Driver {} is not available for robot {}", driver_name, RobotInterface::name());
  }
};

} // namespace mc_rtde

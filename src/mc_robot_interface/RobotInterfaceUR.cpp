#include <mc_robot_interface/RobotInterfaceUR.h>

namespace mc_robot
{

RobotInterfaceUR::RobotInterfaceUR(const std::string & name, const mc_rtc::Configuration & config)
: RobotInterface(name, config)
{
  mc_rtc::log::info("This is happening in the child");
};

} // namespace mc_robot

#pragma once

#include <mc_robot_interface/RobotInterface.h>
#include <string>

namespace mc_robot
{

struct RobotInterfaceFactory
{
public:
private:
  std::unordered_map<std::string, std::unique_ptr<RobotInterface>> robots_interfaces_;
};

} // namespace mc_robot

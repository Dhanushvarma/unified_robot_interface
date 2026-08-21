#include <mc_robot_interface/FMInterfaceTemplate.h>
#include <mc_robot_interface/RobotInterfaceFactory.h>

#include <fmt/core.h>

namespace mc_robot
{

std::unique_ptr<RobotInterfaceBase> RobotInterfaceFactory::makeInterface(const std::string & name,
                                                                         const mc_rtc::Configuration & config,
                                                                         const uint8_t & buffer_size)
{
  fmt::print("[RobotInterfaceFactor] Making Interface for robot: {}\n", name);
  // "interface" selects the manager-side RobotInterfaceBase implementation.
  const std::string interface_type = config("interface", std::string{"interface_template"});

  if(interface_type == "interface_template")
  {
    return std::make_unique<mc_interface_template::FMInterfaceTemplate>(name, config, buffer_size);
  }

  fmt::print("[RobotInterfaceFactor][error] Robot interface type {} is not supported\n", interface_type);

  return nullptr;
}

} // namespace mc_robot

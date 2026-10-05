#pragma once

#include <mc_rtc/Configuration.h>
#include <mc_rtc/logging.h>

namespace robot_interface
{

enum class ControlMode
{
  Position,
  Velocity,
  Torque
};
} // namespace robot_interface

namespace mc_rtc
{

template<>
struct ConfigurationLoader<robot_interface::ControlMode>
{
  static Configuration save(const robot_interface::ControlMode & cm)
  {
    Configuration c;
    switch(cm)
    {
      case robot_interface::ControlMode::Position:
        c.add("cm", "Position");
        break;
      case robot_interface::ControlMode::Velocity:
        c.add("cm", "Velocity");
        break;
      case robot_interface::ControlMode::Torque:
        c.add("cm", "Torque");
        break;
      default:
        log::error_and_throw<std::runtime_error>("ControlMode has unexpected value");
    }
    return c("cm");
  }

  static robot_interface::ControlMode load(const Configuration & conf)
  {
    std::string cm = conf;
    if(cm == "Position")
    {
      return robot_interface::ControlMode::Position;
    }
    if(cm == "Velocity")
    {
      return robot_interface::ControlMode::Velocity;
    }
    if(cm == "Torque")
    {
      return robot_interface::ControlMode::Torque;
    }
    log::error_and_throw<std::runtime_error>("ControlMode has unexpected value {}", cm);
  }
};

} // namespace mc_rtc

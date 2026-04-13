#pragma once

#include <mc_network_interface/NetworkInterfaceFactory.h>

#include <string>

namespace mc_network
{

void NetworkInterfaceFactory::addNetworkInterface(const std::string & name, const mc_rtc::Configuration & config)
{
  const std::string protocol = config("network")("protocol");

  if(protocol == "tcp")
  {
    mc_rtc::log::info("Robot {} protocol {}");
    // auto new_robot = std::make_unique<RobotInterfaceUR>(name, config);
    // auto [it, success] = robots_interfaces_.try_emplace(name, std::move(new_robot));
    // if(!success)
    // {
    //   mc_rtc::log::warning("Robot {} already exists", name);
    // }
  }
  else
  {
    mc_rtc::log::warning("Network protocol {} is not supported", protocol);
  }
}

} // namespace mc_network

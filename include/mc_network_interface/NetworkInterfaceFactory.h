#pragma once

#include <mc_rtc/Configuration.h>
#include <mc_network_interface/NetworkInterface.h>

namespace mc_network
{

struct NetworkInterfaceFactory
{
  void addNetworkInterface(const std::string & name, const mc_rtc::Configuration & config);
  // void addRobotInterface(const std::string & name, const mc_control::Configuration & config);

private:
  std::unordered_map<std::string, std::shared_ptr<mc_network::NetworkInterface>> network_interfaces_;
  // std::unordered_map<std::string, std::unique_ptr<RobotInterface>> robots_interfaces_;
};

} // namespace mc_network

#pragma once

#include <mc_network_interface/NetworkInterface.h>

#include <mc_rtc/Configuration.h>

namespace mc_network
{

struct NetworkInterfaceFactory
{
  void addNetworkInterface(const std::string & name, const mc_rtc::Configuration & config);

private:
  std::unordered_map<std::string, std::unique_ptr<mc_network::NetworkInterface>> network_interfaces_;
};

} // namespace mc_network

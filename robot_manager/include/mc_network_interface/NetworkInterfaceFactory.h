#pragma once

#include <mc_network_interface/NetworkInterface.h>

#include <mc_rtc/Configuration.h>

namespace mc_network
{

struct NetworkInterfaceFactory
{
  bool addNetworkInterface(const std::string & name, const mc_rtc::Configuration & config);

  const std::unordered_map<std::string, std::unique_ptr<mc_network::NetworkInterface>> & networkInterfaces() const
  {
    return network_interfaces_;
  }

private:
  std::unordered_map<std::string, std::unique_ptr<mc_network::NetworkInterface>> network_interfaces_;
};

} // namespace mc_network

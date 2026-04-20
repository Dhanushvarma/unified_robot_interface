#include <mc_network_interface/NetworkInterfaceFactory.h>
#include <mc_network_interface/NetworkInterfaceShm.h>

#include <mc_rtc/logging.h>

#include <string>

namespace mc_network
{

std::unique_ptr<NetworkInterface> NetworkInterfaceFactory::makeNetwork(const mc_rtc::Configuration & network_config)
{
  mc_rtc::log::success("makeNetwork start");
  const std::string protocol = network_config("protocol");

  mc_rtc::log::info("network addNetworkInterface 1");

  if(protocol == "shm")
  {
    return std::make_unique<NetworkInterfaceShm>(network_config);
  }

  mc_rtc::log::error("Network protocol {} is not supported", protocol);
  return nullptr;
}

void NetworkInterfaceFactory::addNetworkInterface(const std::string & name, std::unique_ptr<NetworkInterface> network)
{
  auto [it, success] = network_interfaces_.try_emplace(name, std::move(network));
  if(!success)
  {
    mc_rtc::log::warning("Network for robot {} already exists", name);
  }
}

} // namespace mc_network

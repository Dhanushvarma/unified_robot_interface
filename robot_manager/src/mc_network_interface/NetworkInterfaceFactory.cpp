#include <mc_network_interface/NetworkInterfaceFactory.h>
#include <mc_network_interface/NetworkInterfaceShm.h>

#include <mc_rtc/logging.h>

#include <string>

namespace mc_network
{

std::unique_ptr<NetworkInterface> NetworkInterfaceFactory::makeNetworkInterface(const mc_rtc::Configuration & config)
{
  mc_rtc::log::success("network addNetworkInterface start");

  const mc_rtc::Configuration network_config = config("network");
  const std::string protocol = network_config("protocol");

  std::unique_ptr<NetworkInterface> network;

  mc_rtc::log::info("network addNetworkInterface 1");

  if(protocol == "shm")
  {
    network = std::make_unique<NetworkInterfaceShm>(network_config);
  }
  // else if(protocol == "shm")
  // {
  //   network = std::make_unique<mc_network::NetworkInterfaceShm>(name, config);
  //   //   auto [it, success] = network_interfaces_.try_emplace(name, std::move(network));
  //   //   if(!success)
  //   //   {
  //   //     mc_rtc::log::warning("Network for robot {} already exists", name);
  //   //   }
  // }
  else
  {
    mc_rtc::log::warning("Network protocol {} is not supported", protocol);
  }

  mc_rtc::log::info("network addNetworkInterface done");

  return network;
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

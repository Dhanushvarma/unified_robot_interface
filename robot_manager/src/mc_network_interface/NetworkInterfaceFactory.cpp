#include <mc_network_interface/NetworkInterfaceFactory.h>
#include <mc_network_interface/NetworkInterfaceTcp.h>
#include <mc_network_interface/NetworkInterfaceUdp.h>
#include <mc_network_interface/NetworkInterfaceZenoh.h>

#include <mc_rtc/logging.h>

#include <string>

namespace mc_network
{

bool NetworkInterfaceFactory::addNetworkInterface(const std::string & name, const mc_rtc::Configuration & config)
{
  mc_rtc::log::success("network addNetworkInterface start");

  // QUESTION: when using `const std::string protocol { config("network")("protocol") };`
  // Why do I have this error ?
  // what():  Stored Json value is not an int (error path: ("Robots")("panda1")("network")("protocol"))
  const std::string protocol = config("network")("protocol");

  mc_rtc::log::info("network addNetworkInterface 1");

  if(protocol == "tcp")
  {
    auto new_client = std::make_unique<mc_network::NetworkInterfaceTcp>(name, config);
    auto [it, success] = network_interfaces_.try_emplace(name, std::move(new_client));
    if(!success)
    {
      mc_rtc::log::warning("Network for robot {} already exists", name);
    }
  }
  else
  {
    mc_rtc::log::warning("Network protocol {} is not supported", protocol);
    return false;
  }

  mc_rtc::log::info("network addNetworkInterface done");

  return true;
}

} // namespace mc_network

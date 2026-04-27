#include <mc_network_interface/NetworkInterfaceZenoh.h>
#include <mc_robot_interface/InterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <thread>

namespace mc_interface_template
{

InterfaceTemplate::InterfaceTemplate(const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("InterfaceTemplate remote start");

  mc_rtc::Configuration network_config("../etc/network.yaml");
  // network_ = mc_network::NetworkInterfaceFactory::makeNetwork(network_config);
  network_ = std::make_unique<mc_network::NetworkInterfaceZenoh>(network_config);

  bool got_config = false;
  mc_network::MessageConfig msgConfig;

  // while(!got_config && !interrupt)
  // {
  //   mc_rtc::log::info("[mc_network] Waiting for config from robot manager");
  //   got_config = network_->receiveMessage(msgConfig);
  //   if(!got_config)
  //   {
  //     std::this_thread::sleep_for(std::chrono::seconds(2));
  //   }
  // }

  if(interrupt)
  {
    mc_rtc::log::warning("Initialization interrupted");
    return;
  }

  mc_rtc::log::info("InterfaceTemplate done");
};

InterfaceTemplate::InterfaceTemplate(const std::string & name,
                                     const mc_rtc::Configuration & config,
                                     uint8_t buffer_size)
: RobotInterface(name, config, (buffer_size == 0) ? 6 : buffer_size)
{
  mc_rtc::log::success("RobotInterfaceTemplate manager start");

  mc_rtc::Configuration network_config(config("network"));
  // network_ = std::make_unique<mc_network::NetworkInterfaceZenoh>(network_config, RobotInterface::name_);
  network_ = mc_network::NetworkInterfaceFactory::makeNetwork(network_config);

  mc_rtc::log::info("RobotInterfaceTemplate done");
};

} // namespace mc_interface_template

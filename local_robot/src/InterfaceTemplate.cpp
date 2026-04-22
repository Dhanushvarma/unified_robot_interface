#include <InterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <thread>

namespace mc_interface_template
{

InterfaceTemplate::InterfaceTemplate(const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("InterfaceTemplate start");

  mc_rtc::Configuration network_config("../etc/network.yaml");
  network_ = mc_network::NetworkInterfaceFactory::makeNetwork(network_config);

  bool got_config = false;
  mc_network::MessageConfig msgConfig;

  // FIX: Include the global interrupt atomic in your loop condition
  // You may need to pass this reference or access it globally
  while(!got_config && !interrupt)
  {
    got_config = network_->receiveMessage(msgConfig);
    if(!got_config)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

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
  mc_rtc::log::success("RobotInterfaceTemplate start");
  mc_rtc::log::info("RobotInterfaceTemplate done");
};

} // namespace mc_interface_template

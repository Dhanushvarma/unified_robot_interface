#include <Interface.h>

#include <mc_rtc/logging.h>

#include <thread>

namespace mc_interface
{

RobotInterfaceTemplate::RobotInterfaceTemplate()
{
  mc_rtc::log::success("RobotInterfaceTemplate start");
  // TODO:
  // from etc/mc_rtc.yaml watchout for new information there
  // when receive, start the interface
  mc_rtc::Configuration network_config("../etc/network.yaml");
  // TODO: setup network according to yaml file. then listen and wait

  network_ = mc_network::NetworkInterfaceFactory::makeNetwork(network_config);
  bool got_config = false;
  mc_network::MessageConfig msgConfig;
  do
  {
    got_config = network_->receiveMessage(msgConfig);
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
  } while(!got_config);

  // const Protocol protocol_test = config("network")("protocol");
  // mc_rtc::log::info("protocol_test = {}", protocol_test);

  mc_rtc::log::info("RobotInterfaceTemplate done");
};

RobotInterfaceTemplate::RobotInterfaceTemplate(const std::string & name,
                                               const mc_rtc::Configuration & config,
                                               uint8_t buffer_size)
: RobotInterface(name, config, (buffer_size == 0) ? 6 : buffer_size)
{
  mc_rtc::log::success("RobotInterfaceTemplate start");
  mc_rtc::log::info("RobotInterfaceTemplate done");
};

} // namespace mc_interface

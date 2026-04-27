#include <mc_communication/CommunicationZenoh.h>
#include <mc_robot_interface/InterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <thread>

namespace mc_interface_template
{

InterfaceTemplate::InterfaceTemplate(const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("InterfaceTemplate remote start");

  mc_rtc::Configuration com_config("../etc/communication.yaml");
  if(com_config.has("name"))
  {
    communication_ = mc_communication::CommunicationFactory::makeCommunication(com_config("name"), com_config);
  }
  else
  {
    mc_rtc::log::error_and_throw("Missing name of robot");
  }

  bool got_config = false;
  mc_communication::MessageConfig msgConfig;

  // while(!got_config && !interrupt)
  // {
  //   mc_rtc::log::info("[mc_communication] Waiting for config from robot manager");
  //   got_config = communication_->receiveMessage(msgConfig);
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

  mc_rtc::Configuration com_config(config("communication"));
  // communication_ = std::make_unique<mc_communication::CommunicationZenoh>(com_config, RobotInterface::name_);
  communication_ = mc_communication::CommunicationFactory::makeCommunication(name, com_config);

  mc_rtc::log::info("RobotInterfaceTemplate done");
};

} // namespace mc_interface_template

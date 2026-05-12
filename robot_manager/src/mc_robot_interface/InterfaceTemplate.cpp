#include <mc_communication/CommunicationZenoh.h>
#include <mc_robot_interface/InterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <thread>

namespace mc_interface_template
{

/* From manager */
InterfaceTemplate::InterfaceTemplate(const std::string & name,
                                     const mc_rtc::Configuration & config,
                                     uint8_t buffer_size)
: RobotInterface(name, config, (buffer_size == 0) ? 6 : buffer_size)
{
  mc_rtc::log::success("RobotInterfaceTemplate manager start");

  mc_rtc::Configuration com_config(config("communication"));
  communication_ = mc_communication::CommunicationFactory::makeCommunicationSever(name, com_config);

  mc_rtc::log::info("RobotInterfaceTemplate done");
};

/* Local robot */
InterfaceTemplate::InterfaceTemplate(const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("InterfaceTemplate remote start");

  mc_rtc::Configuration com_config("local_robot/etc/communication.yaml");
  if(com_config.has("name"))
  {
    communication_ = mc_communication::CommunicationFactory::makeCommunicationClient(com_config);
  }
  else
  {
    mc_rtc::log::error_and_throw("Missing name of robot");
  }

  bool got_config = false;

  while(!got_config && !interrupt)
  {
    mc_rtc::log::info("[mc_communication] Waiting for config from robot manager");
    got_config = communication_->receiveMessage(mc_communication::Communication::MessageType::CONFIG);
    if(!got_config)
    {
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  }

  if(interrupt)
  {
    mc_rtc::log::warning("Initialization interrupted");
    return;
  }

  if(!communication_->latestConfig().empty())
  {
    mc_rtc::log::success("HERE IS CONFIG");
    mc_rtc::log::info(communication_->latestConfig().dump(true, true));
  }
  mc_rtc::log::info("InterfaceTemplate local done");
};

} // namespace mc_interface_template

#include <mc_communication/CommunicationZenoh.h>
#include <mc_robot_interface/InterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <thread>

namespace mc_interface_template
{

InterfaceTemplate::InterfaceTemplate(const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("InterfaceTemplate remote start");

  mc_rtc::Configuration com_config("local_robot/etc/communication.yaml");
  if(com_config.has("name"))
  {
    setCommunication(mc_communication::CommunicationFactory::makeCommunicationClient(com_config));
  }
  else
  {
    mc_rtc::log::error_and_throw("Missing name of robot");
  }

  bool got_config = false;

  while(!got_config && !interrupt)
  {
    mc_rtc::log::info("[mc_communication] Waiting for config from robot manager");
    got_config = communication().receiveMessage(mc_communication::Communication::MessageType::CONFIG);
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

  if(!communication().latestConfig().empty())
  {
    mc_rtc::log::success("HERE IS CONFIG");
    mc_rtc::log::info(communication().latestConfig().dump(true, true));
  }
  mc_rtc::log::info("InterfaceTemplate local done");
};

void InterfaceTemplate::updateSensors() {
  // TODO: read from sensor and send states to manager
};

void InterfaceTemplate::updateControl() {
  // TODO: receive commands from manager
};

} // namespace mc_interface_template

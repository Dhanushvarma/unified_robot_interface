#include <mc_communication/CommunicationZenoh.h>
#include <mc_robot_interface/FMInterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <thread>

namespace mc_interface_template
{

FMInterfaceTemplate::FMInterfaceTemplate(const std::string & name,
                                         const mc_rtc::Configuration & config,
                                         uint8_t buffer_size)
: RobotInterfaceBase(name, config, (buffer_size == 0) ? 6 : buffer_size)
{
  mc_rtc::log::success("FMInterfaceTemplate manager start");

  mc_rtc::Configuration com_config(config("communication"));
  setCommunication(mc_communication::CommunicationFactory::makeCommunicationSever(name, com_config));

  mc_rtc::log::info("FMInterfaceTemplate done");
};

void FMInterfaceTemplate::updateSensors()
{
  if(communication().receiveMessage(mc_communication::Communication::MessageType::STATE))
  {
    auto latest_state = communication().latestState();
    mc_rtc::log::success("Received state");
  }
  else
  {
    mc_rtc::log::error("Trouble receiving state");
  }
};

void FMInterfaceTemplate::updateControl() {
  // TODO: send command to local robot
};

} // namespace mc_interface_template

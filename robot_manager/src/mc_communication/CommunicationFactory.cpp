#include <mc_communication/CommunicationFactory.h>
#include <mc_communication/CommunicationZenoh.h>

#include <mc_rtc/logging.h>

#include <regex>

namespace mc_communication
{

std::unique_ptr<Communication> CommunicationFactory::makeCommunication(const std::string & name,
                                                                       const mc_rtc::Configuration & com_config)
{
  mc_rtc::log::success("communication makeCommunication start");
  const std::string protocol = com_config("protocol");

  mc_rtc::log::info("communication makeCommunication 1");

  std::regex zenoh("zenoh");

  if(std::regex_search(protocol, zenoh))
  {
    return std::make_unique<CommunicationZenoh>(com_config);
  }

  mc_rtc::log::error("Communication protocol {} is not supported", protocol);
  return nullptr;
}

} // namespace mc_communication

#include <mc_communication/CommunicationFactory.h>
#include <mc_communication/CommunicationZenoh.h>

#include <mc_rtc/logging.h>

#include <regex>

namespace mc_communication
{

std::unique_ptr<Communication> CommunicationFactory::makeCommunicationSever(const std::string & name,
                                                                            const mc_rtc::Configuration & mc_config,
                                                                            const std::string & com_config_path)
{
  mc_rtc::log::success("communication makeCommunicationSever start");
  const std::string protocol = mc_config("protocol");

  mc_rtc::log::info("communication makeCommunicationSever 1");

  std::regex zenoh("zenoh");

  if(std::regex_search(protocol, zenoh))
  {
    return std::make_unique<CommunicationSeverZenoh>(name, mc_config, com_config_path);
  }

  mc_rtc::log::error("Communication protocol {} is not supported", protocol);
  return nullptr;
}

std::unique_ptr<Communication> CommunicationFactory::makeCommunicationClient(const mc_rtc::Configuration & mc_config,
                                                                             const std::string & com_config_path)
{
  mc_rtc::log::success("communication makeCommunicationClient start");
  const std::string protocol = mc_config("protocol");

  mc_rtc::log::info("communication makeCommunicationClient 1");

  std::regex zenoh("zenoh");

  if(std::regex_search(protocol, zenoh))
  {
    return std::make_unique<CommunicationClientZenoh>(mc_config("name"), mc_config, com_config_path);
  }

  mc_rtc::log::error("Communication protocol {} is not supported", protocol);
  return nullptr;
}

} // namespace mc_communication

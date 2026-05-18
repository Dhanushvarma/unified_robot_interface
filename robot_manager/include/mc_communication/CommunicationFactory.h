#pragma once

#include <mc_communication/Communication.h>

#include <mc_rtc/Configuration.h>

namespace mc_communication
{

struct CommunicationFactory
{
  void checkCompatibility();
  static std::unique_ptr<Communication> makeCommunicationSever(const std::string & name,
                                                               const mc_rtc::Configuration & mc_config,
                                                               const std::string & com_config_path = "");
  static std::unique_ptr<Communication> makeCommunicationClient(const mc_rtc::Configuration & mc_config,
                                                                const std::string & com_config_path = "");
};

} // namespace mc_communication

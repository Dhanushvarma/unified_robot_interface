#pragma once

#include <mc_communication/Communication.h>

#include <mc_rtc/Configuration.h>

namespace mc_communication
{

struct CommunicationFactory
{
  void checkCompatibility();
  static std::unique_ptr<Communication> makeCommunication(const std::string & name,
                                                          const mc_rtc::Configuration & config);
};

} // namespace mc_communication

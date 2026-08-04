#pragma once

#include <robot_comm/Communication.h>

#include <mc_rtc/Configuration.h>

namespace robot_comm
{

struct CommunicationFactory
{
  void checkCompatibility();

  static std::unique_ptr<Communication> makeCommunication(const std::string & name,
                                                          const mc_rtc::Configuration & config);
};

} // namespace robot_comm

#pragma once

#include <robot_comm/Communication.h>

#include <mc_rtc/Configuration.h>

namespace robot_comm
{

/// Creates the Communication matching a `network_interface` configuration.
struct CommunicationFactory
{
  void checkCompatibility();

  /// Create the Communication for robot `name`. Throws if the configured
  /// protocol is not supported.
  static std::unique_ptr<Communication> makeCommunication(const std::string & name,
                                                          const mc_rtc::Configuration & config);
};

} // namespace robot_comm

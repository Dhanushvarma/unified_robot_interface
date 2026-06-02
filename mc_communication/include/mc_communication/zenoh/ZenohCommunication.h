#pragma once

#include <mc_communication/Communication.h>
#include <mc_communication/zenoh/ZenohTransport.h>

namespace mc_communication
{

class ZenohCommunication : public Communication
{
public:
  ZenohCommunication(std::string name, const mc_rtc::Configuration & config);

  ~ZenohCommunication() override = default;
};

} // namespace mc_communication

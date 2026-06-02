#pragma once

#include <mc_communication/Communication.h>
#include <mc_communication/shm/ShmTransport.h>

namespace mc_communication
{

class ShmCommunication : public Communication
{
public:
  ShmCommunication(std::string name, const mc_rtc::Configuration & config);

  ~ShmCommunication() override = default;
};

} // namespace mc_communication

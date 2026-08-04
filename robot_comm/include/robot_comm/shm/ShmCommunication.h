#pragma once

#include <robot_comm/Communication.h>
#include <robot_comm/shm/ShmTransport.h>

namespace robot_comm
{

class ShmCommunication : public Communication
{
public:
  ShmCommunication(std::string name, const mc_rtc::Configuration & config);

  ~ShmCommunication() override = default;
};

} // namespace robot_comm

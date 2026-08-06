#pragma once

#include <robot_comm/Communication.h>
#include <robot_comm/zenoh/ZenohTransport.h>

namespace robot_comm
{

class ZenohCommunication : public Communication
{
public:
  ZenohCommunication(std::string name, const mc_rtc::Configuration & config);

  ~ZenohCommunication() override = default;
};

} // namespace robot_comm

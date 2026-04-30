#pragma once

#include <mc_communication/flatbuffers/Message_generated.h>

#include <mc_rtc/Configuration.h>

#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace mc_communication
{

class Communication
{
public:
  Communication() : Communication(mc_rtc::Configuration("../etc/communication.yaml")) {};

  Communication(const mc_rtc::Configuration & com_config) : ip_(com_config("ip")), port_(com_config("port")) {};

  virtual ~Communication() = default;
  Communication(const Communication &) = delete;
  Communication & operator=(const Communication &) = delete;
  Communication(Communication &&) = delete;
  Communication & operator=(Communication &&) = delete;

  virtual bool sendMessage(const uint8_t * data, size_t size) = 0;
  virtual flatbuffers::FlatBufferBuilder serialize(const mc_rtc::Configuration & config) = 0;

  virtual bool receiveMessage(const uint8_t * data, size_t size) = 0;

protected:
  [[nodiscard]] const std::string & ip() const
  {
    return ip_;
  }
  [[nodiscard]] const uint16_t port() const
  {
    return port_;
  }

private:
  const std::string name_;
  const std::string ip_;
  const uint16_t port_;
};

} // namespace mc_communication

#pragma once

#include <mc_communication/Communication.h>

// #include <mc_rtc/logging.h>

// #include "zenoh.hxx"

#include <memory>
// #include <mutex>
// #include <optional>
#include <string>

namespace mc_communication
{

class CommunicationSeverZenoh : public Communication
{
public:
  CommunicationSeverZenoh();
  CommunicationSeverZenoh(const std::string & name,
                          const mc_rtc::Configuration & mc_config,
                          const std::string & com_config_path = "");
  ~CommunicationSeverZenoh() override;

  CommunicationSeverZenoh(const CommunicationSeverZenoh &) = delete;
  CommunicationSeverZenoh & operator=(const CommunicationSeverZenoh &) = delete;
  CommunicationSeverZenoh(CommunicationSeverZenoh &&) = delete;
  CommunicationSeverZenoh & operator=(CommunicationSeverZenoh &&) = delete;

  bool sendMessage(Communication::MessageType type, const uint8_t * data, size_t size) override;
  bool receiveMessage(Communication::MessageType type) override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

class CommunicationClientZenoh : public Communication
{
public:
  CommunicationClientZenoh();
  CommunicationClientZenoh(const std::string & name,
                           const mc_rtc::Configuration & mc_config,
                           const std::string & com_config_path = "");
  ~CommunicationClientZenoh() override;

  CommunicationClientZenoh(const CommunicationClientZenoh &) = delete;
  CommunicationClientZenoh & operator=(const CommunicationClientZenoh &) = delete;
  CommunicationClientZenoh(CommunicationClientZenoh &&) = delete;
  CommunicationClientZenoh & operator=(CommunicationClientZenoh &&) = delete;

  bool sendMessage(Communication::MessageType type, const uint8_t * data, size_t size) override;
  bool receiveMessage(Communication::MessageType type) override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace mc_communication

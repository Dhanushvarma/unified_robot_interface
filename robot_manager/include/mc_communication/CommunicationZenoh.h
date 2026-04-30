#pragma once

#include <mc_communication/Communication.h>

#include <mc_rtc/logging.h>

#include "zenoh.hxx"

#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace mc_communication
{

class CommunicationSeverZenoh : public Communication
{
public:
  CommunicationSeverZenoh();
  CommunicationSeverZenoh(const mc_rtc::Configuration & com_config);
  ~CommunicationSeverZenoh() override;

  bool sendMessage(const uint8_t * data, size_t size) override;
  flatbuffers::FlatBufferBuilder serialize(const mc_rtc::Configuration & config) override;

  bool receiveMessage(const uint8_t * data, size_t size) override
  {
    return false;
  };

private:
  zenoh::Config configureTransport(const mc_rtc::Configuration & com_config);
  void setupTransport(const mc_rtc::Configuration & com_config);

  std::unique_ptr<zenoh::Session> session_;
  std::optional<zenoh::Querier> config_pub_;
  std::optional<zenoh::Subscriber<void>> state_sub_;
  std::optional<zenoh::Publisher> command_pub_;

  // Cached messages from subscribers
  std::mutex mutex_;
  std::optional<MessageConfig> latest_config_;
  std::optional<MessageState> latest_state_;
  std::optional<MessageCommand> latest_command_;
};

class CommunicationClientZenoh : public Communication
{
public:
  CommunicationClientZenoh();
  CommunicationClientZenoh(const mc_rtc::Configuration & com_config);
  ~CommunicationClientZenoh() override;

  bool sendMessage(const uint8_t * data, size_t size) override;

  bool receiveMessage(const uint8_t * data, size_t size) override
  {
    return false;
  };

private:
  zenoh::Config configureTransport(const mc_rtc::Configuration & com_config);
  void setupTransport(const mc_rtc::Configuration & com_config);

  std::unique_ptr<zenoh::Session> session_;
  std::optional<zenoh::Publisher> state_pub_;
  std::optional<zenoh::Subscriber<void>> command_sub_;

  // Cached messages from subscribers
  std::mutex mutex_;
  std::optional<MessageConfig> latest_config_;
  std::optional<MessageState> latest_state_;
  std::optional<MessageCommand> latest_command_;
};

} // namespace mc_communication

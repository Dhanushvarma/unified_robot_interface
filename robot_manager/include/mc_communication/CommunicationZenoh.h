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

class CommunicationZenoh : public Communication
{
public:
  CommunicationZenoh();
  CommunicationZenoh(const mc_rtc::Configuration & com_config);
  ~CommunicationZenoh() override;

  bool sendMessage(const std::string & message) override;
  bool receiveMessage(std::string & message) override;

private:
  void configureTransport(const mc_rtc::Configuration & com_config, zenoh::Config & zenoh_config);
  void setupPubSub(const mc_rtc::Configuration & com_config);

  std::unique_ptr<zenoh::Session> session_;
  std::optional<zenoh::Querier> config_pub_;
  std::optional<zenoh::Publisher> state_pub_;
  std::optional<zenoh::Subscriber<void>> state_sub_;
  std::optional<zenoh::Publisher> command_pub_;
  std::optional<zenoh::Subscriber<void>> command_sub_;

  // Cached messages from subscribers
  std::mutex mutex_;
  std::optional<MessageConfig> latest_config_;
  std::optional<MessageState> latest_state_;
  std::optional<MessageCommand> latest_command_;
};

} // namespace mc_communication

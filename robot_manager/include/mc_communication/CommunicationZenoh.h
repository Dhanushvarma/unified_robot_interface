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
  CommunicationSeverZenoh(const std::string & name, const mc_rtc::Configuration & com_config);
  ~CommunicationSeverZenoh() override;

  CommunicationSeverZenoh(const CommunicationSeverZenoh &) = delete;
  CommunicationSeverZenoh & operator=(const CommunicationSeverZenoh &) = delete;
  CommunicationSeverZenoh(CommunicationSeverZenoh &&) = delete;
  CommunicationSeverZenoh & operator=(CommunicationSeverZenoh &&) = delete;

  bool sendMessage(Communication::MessageType type, const uint8_t * data, size_t size) override;

  bool receiveMessage(Communication::MessageType type) override;

private:
  static zenoh::Config configureTransport(const mc_rtc::Configuration & com_config);
  void setupTransport(const mc_rtc::Configuration & com_config);

  std::unique_ptr<zenoh::Session> session_;
  std::optional<zenoh::Queryable<void>> config_queryable_;
  std::vector<uint8_t> config_cache_;
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
  CommunicationClientZenoh(const std::string & name, const mc_rtc::Configuration & com_config);
  ~CommunicationClientZenoh() override;

  CommunicationClientZenoh(const CommunicationClientZenoh &) = delete;
  CommunicationClientZenoh & operator=(const CommunicationClientZenoh &) = delete;
  CommunicationClientZenoh(CommunicationClientZenoh &&) = delete;
  CommunicationClientZenoh & operator=(CommunicationClientZenoh &&) = delete;

  bool sendMessage(Communication::MessageType type, const uint8_t * data, size_t size) override;

  bool receiveMessage(Communication::MessageType type) override;

private:
  static zenoh::Config configureTransport(const mc_rtc::Configuration & com_config);
  void setupTransport(const mc_rtc::Configuration & com_config);

  std::unique_ptr<zenoh::Session> session_;
  std::optional<zenoh::Querier> config_querier_;
  std::optional<zenoh::Publisher> state_pub_;
  std::optional<zenoh::Subscriber<void>> command_sub_;
};

} // namespace mc_communication

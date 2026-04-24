#pragma once

#include <mc_rtc/logging.h>
#include <mc_network_interface/NetworkInterface.h>

#include "zenoh.hxx"

#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace mc_network
{

class NetworkInterfaceZenoh : public NetworkInterface
{
public:
  NetworkInterfaceZenoh();
  NetworkInterfaceZenoh(const mc_rtc::Configuration & network_config, const std::string & name = "");
  ~NetworkInterfaceZenoh() override;

  bool sendMessage(const MessageConfig & msg) override;
  bool sendMessage(const MessageState & msg) override;
  bool sendMessage(const MessageCommand & msg) override;

  bool receiveMessage(MessageConfig & msg) override;
  bool receiveMessage(MessageState & msg) override;
  bool receiveMessage(MessageCommand & msg) override;

private:
  void configureShmTransport(zenoh::Config & zenoh_config);
  void configureTcpTransport(zenoh::Config & zenoh_config);
  void configureUdpTransport(zenoh::Config & zenoh_config);
  void setupPubSub(const std::string & name);

  // Serialization helpers
  std::vector<uint8_t> serialize(const MessageConfig & msg);
  std::vector<uint8_t> serialize(const MessageState & msg);
  std::vector<uint8_t> serialize(const MessageCommand & msg);

  bool deserialize(const std::vector<uint8_t> & data, MessageConfig & msg);
  bool deserialize(const std::vector<uint8_t> & data, MessageState & msg);
  bool deserialize(const std::vector<uint8_t> & data, MessageCommand & msg);

  // Zenoh session and publishers (moved to unique_ptr to avoid default constructor issues)
  std::unique_ptr<zenoh::Session> session_;
  std::optional<zenoh::Publisher> config_pub_;
  std::optional<zenoh::Publisher> state_pub_;
  std::optional<zenoh::Publisher> command_pub_;

  // Subscribers
  std::optional<zenoh::Subscriber<void>> config_sub_;
  std::optional<zenoh::Subscriber<void>> state_sub_;
  std::optional<zenoh::Subscriber<void>> command_sub_;

  // Cached messages from subscribers
  std::mutex mutex_;
  std::optional<MessageConfig> latest_config_;
  std::optional<MessageState> latest_state_;
  std::optional<MessageCommand> latest_command_;
};

} // namespace mc_network

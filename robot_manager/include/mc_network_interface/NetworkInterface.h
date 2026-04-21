#pragma once

#include <mc_rtc/Configuration.h>

#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace mc_network
{

struct MessageConfig
{
  std::string name;
  mc_rtc::Configuration config;

  bool read = true;
};

struct MessageState
{
  std::vector<double> state;
};

struct MessageCommand
{
  std::vector<double> command;
};

class NetworkInterface
{
public:
  // TODO: move NetworkInterface() here
  // Look for ../etc/network.yaml
  NetworkInterface() : NetworkInterface(mc_rtc::Configuration("../etc/network.yaml")) {};

  NetworkInterface(const mc_rtc::Configuration & network_config) : ip_(network_config("ip"))
  {
    std::vector<uint16_t> config_port = network_config("port");
    ports_["config"] = config_port[0];
    ports_["state"] = config_port[1];
    ports_["command"] = config_port[2];
  };

  virtual ~NetworkInterface() = default;
  NetworkInterface(const NetworkInterface &) = delete;
  NetworkInterface & operator=(const NetworkInterface &) = delete;
  NetworkInterface(NetworkInterface &&) = delete;
  NetworkInterface & operator=(NetworkInterface &&) = delete;

  // Send messages
  virtual void sendMessage(const MessageConfig & msg) = 0;
  virtual void sendMessage(const MessageState & msg) = 0;
  virtual void sendMessage(const MessageCommand & msg) = 0;

  // Receive messages
  virtual bool receiveMessage(MessageConfig & msg) = 0;
  virtual bool receiveMessage(MessageState & msg) = 0;
  virtual bool receiveMessage(MessageCommand & msg) = 0;

protected:
  [[nodiscard]] const std::string & ip() const
  {
    return ip_;
  }
  [[nodiscard]] std::unordered_map<std::string, uint16_t> ports() const
  {
    return ports_;
  }

private:
  const std::string ip_;
  std::unordered_map<std::string, uint16_t> ports_;
};

} // namespace mc_network

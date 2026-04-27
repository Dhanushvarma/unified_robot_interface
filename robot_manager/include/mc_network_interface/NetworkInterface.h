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

  NetworkInterface(const mc_rtc::Configuration & network_config)
  : ip_(network_config("ip")), port_(network_config("port")) {};

  virtual ~NetworkInterface() = default;
  NetworkInterface(const NetworkInterface &) = delete;
  NetworkInterface & operator=(const NetworkInterface &) = delete;
  NetworkInterface(NetworkInterface &&) = delete;
  NetworkInterface & operator=(NetworkInterface &&) = delete;

  virtual bool sendMessage(const std::string & message) = 0;
  virtual bool receiveMessage(std::string & message) = 0;

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

} // namespace mc_network

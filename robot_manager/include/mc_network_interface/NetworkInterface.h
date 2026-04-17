#pragma once

#include <mc_rtc/Configuration.h>

#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace mc_network
{

struct MessageConfig
{
  bool read = true;
  std::string name;
  mc_rtc::Configuration config;
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
  : ip_(network_config("ip")), ports_(network_config("port")) {};

protected:
  [[nodiscard]] const std::string & ip() const
  {
    return ip_;
  }
  [[nodiscard]] std::vector<uint16_t> ports() const
  {
    return ports_;
  }
  [[nodiscard]] uint16_t port(const std::string & category) const
  {
    if(category == "config")
      return ports_[0];
    else if(category == "state")
      return ports_[1];
    else
      return ports_[2];
  }

private:
  const std::string ip_;
  std::vector<uint16_t> ports_;
};

} // namespace mc_network

#pragma once

#include <string>

namespace mc_network
{

struct NetworkInterfaceSever
{

private:
};

class NetworkInterface
{
public:
  NetworkInterface(std::string name, std::string ip, uint16_t port)
  : name_(std::move(name)), ip_(std::move(ip)), port_(port) {};

protected:
  // Accessors for derived classes
  [[nodiscard]] const std::string & name() const
  {
    return name_;
  }
  [[nodiscard]] const std::string & ip() const
  {
    return ip_;
  }
  [[nodiscard]] uint16_t port() const
  {
    return port_;
  }

private:
  const std::string name_;
  const std::string ip_;
  uint16_t port_;
};

} // namespace mc_network

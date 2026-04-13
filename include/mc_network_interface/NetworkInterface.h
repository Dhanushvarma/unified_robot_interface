#pragma once

#include <string>

namespace mc_network
{

struct NetworkInterfaceSever
{

private:
};

struct NetworkInterface
{
  NetworkInterface(const std::string & name, const std::string & ip, const uint16_t & port)
  : name_(name), ip_(ip), port_(port) {};

protected:
  const std::string name_;
  const std::string ip_;
  const uint16_t port_;
};

} // namespace mc_network

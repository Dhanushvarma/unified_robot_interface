#pragma once

#include <mc_network_interface/NetworkInterface.h>

#include <mc_rtc/logging.h>

#include <string>

namespace mc_network
{

struct NetworkInterfaceTcp : public NetworkInterface
{
  NetworkInterfaceTcp(const std::string & name, const std::string & ip, const uint16_t & port)
  : NetworkInterface(name, ip, port)
  {
    mc_rtc::log::success("network Tcp start");

    mc_rtc::log::info("name {}", NetworkInterface::name());
    mc_rtc::log::info("ip {}", NetworkInterface::ip());
    mc_rtc::log::info("port {}", NetworkInterface::port());

    mc_rtc::log::info("network Tcp done");
  };
};

} // namespace mc_network

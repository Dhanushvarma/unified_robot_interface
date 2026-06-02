#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <zenoh.hxx>

#include <mc_communication/transport/TransportInterface.h>

namespace mc_communication
{

class ZenohTransport : public TransportInterface
{
public:
  ZenohTransport(const mc_rtc::Configuration & config);

  ~ZenohTransport() override = default;

  void start() override;

  void stop() override;

  bool publish(const std::string & topic, const ByteBuffer & payload) override;

  void subscribe(const std::string & topic, ReceiveCallback callback) override;

  bool hasSubscriber(const std::string & topic) const override;

private:
  zenoh::Config configureTransport(const mc_rtc::Configuration & config);

private:
  std::unique_ptr<zenoh::Session> session_;

  std::unordered_map<std::string, zenoh::Publisher> publishers_;

  std::unordered_map<std::string, zenoh::Subscriber<void>> subscribers_;
};

} // namespace mc_communication

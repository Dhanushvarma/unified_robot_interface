#pragma once

#include <chrono>
#include <memory>
#include <optional>
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

  // ── Query/reply support ──

  /// Server: register a queryable that responds with data when queried
  void registerQueryable(const std::string & topic, QueryHandler handler) override;

  /// Client: query a topic and wait for a reply (with timeout)
  std::optional<ByteBuffer> query(const std::string & topic,
                                  std::chrono::milliseconds timeout = std::chrono::seconds(1)) override;

private:
  zenoh::Config configureTransport(const mc_rtc::Configuration & config);

  std::unique_ptr<zenoh::Session> session_;

  std::unordered_map<std::string, zenoh::Publisher> publishers_;

  std::unordered_map<std::string, zenoh::Subscriber<void>> subscribers_;

  std::unordered_map<std::string, zenoh::Queryable<void>> queryables_;
};

} // namespace mc_communication

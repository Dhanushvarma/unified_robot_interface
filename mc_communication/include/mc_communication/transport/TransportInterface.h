#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <mc_rtc/Configuration.h>

#include <mc_communication/serialization/Buffer.h>

namespace mc_communication
{

using ReceiveCallback = std::function<void(const std::string & topic, const ByteBuffer & payload)>;
using QueryHandler = std::function<ByteBuffer(const std::string & topic)>;

class TransportInterface
{
public:
  TransportInterface(const mc_rtc::Configuration & config) : config_(config) {}

  virtual ~TransportInterface() = default;

  virtual void start() = 0;

  virtual void stop() = 0;

  virtual bool publish(const std::string & topic, const ByteBuffer & payload) = 0;

  virtual void subscribe(const std::string & topic, ReceiveCallback callback) = 0;

  virtual bool hasSubscriber(const std::string & topic) const = 0;

  virtual void registerQueryable(const std::string & topic, QueryHandler handler) = 0;

  virtual std::optional<ByteBuffer> query(const std::string & topic,
                                          std::chrono::milliseconds timeout = std::chrono::seconds(1)) = 0;

protected:
  mc_rtc::Configuration config_;
};

} // namespace mc_communication

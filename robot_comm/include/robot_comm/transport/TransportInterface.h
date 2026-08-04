#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <mc_rtc/Configuration.h>

#include <robot_comm/serialization/Buffer.h>

namespace robot_comm
{

using ReceiveCallback = std::function<void(const std::string & topic, const ByteBuffer & payload)>;

// Synchronous query handler: receives payload, returns reply payload.
using QueryHandler = std::function<ByteBuffer(const ByteBuffer & payload)>;

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

  // Register a queryable: incoming queries on key_expr are answered synchronously by handler.
  virtual void registerQueryable(const std::string & key_expr, QueryHandler handler) = 0;

  // Send a blocking query and return the first reply payload, or nullopt on timeout.
  virtual std::optional<ByteBuffer> query(const std::string & key_expr,
                                          const ByteBuffer & payload,
                                          std::chrono::milliseconds timeout = std::chrono::seconds(5)) = 0;

protected:
  mc_rtc::Configuration config_;
};

} // namespace robot_comm

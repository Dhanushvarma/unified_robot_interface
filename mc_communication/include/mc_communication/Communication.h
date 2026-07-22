#pragma once

#include <mc_communication/Publisher.h>
#include <mc_communication/Subscriber.h>
#include <mc_communication/serialization/Serializer.h>
#include <mc_communication/transport/TransportInterface.h>

#include <mc_rtc/Configuration.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc_communication
{

class Communication
{
public:
  Communication(std::string name,
                const mc_rtc::Configuration & config,
                SerializationBackend backend = SerializationBackend::Flatbuffer);

  virtual ~Communication();

  void start();
  void stop();

  template<typename T>
  bool publish(const std::string & topic, const T & msg);

  template<typename T>
  std::shared_ptr<Publisher<T>> createPublisher(const std::string & topic);

  template<typename T, typename Callback>
  std::shared_ptr<Subscriber<T>> subscribe(const std::string & topic, Callback && cb);

  template<typename T, typename Handler>
  void registerQueryable(const std::string & topic, Handler && handler);

  template<typename T>
  std::optional<T> query(const std::string & topic, std::chrono::milliseconds timeout = std::chrono::seconds(1));

  void dispatch(const std::string & topic, const ByteBuffer & payload);

  inline std::shared_ptr<ISerializer> serializer() const
  {
    return serializer_;
  }

protected:
  mc_rtc::Configuration parseConfig(const mc_rtc::Configuration & config);

  SerializationBackend backendFromString(const std::string & backend);

  Protocol protocolFromString(const std::string & protocol);

protected:
  std::string name_;

  std::shared_ptr<ISerializer> serializer_;

  std::shared_ptr<TransportInterface> transport_;

  std::unordered_map<std::string, std::vector<std::shared_ptr<SubscriberBase>>> subscribers_;

  mutable std::mutex mutex_;

  std::atomic<bool> running_ = false;
};

///------------------------------------------------------------
/// Typed publish
///------------------------------------------------------------

template<typename T>
bool Communication::publish(const std::string & topic, const T & msg)
{
  auto payload = serializer_->serialize(msg);

  return transport_->publish(topic, payload);
}

///------------------------------------------------------------
/// Publisher creation
///------------------------------------------------------------

template<typename T>
std::shared_ptr<Publisher<T>> Communication::createPublisher(const std::string & topic)
{
  return std::make_shared<Publisher<T>>(*this, topic);
}

///------------------------------------------------------------
/// Subscriber creation
///------------------------------------------------------------

template<typename T, typename Callback>
std::shared_ptr<Subscriber<T>> Communication::subscribe(const std::string & topic, Callback && cb)
{
  auto subscriber = std::make_shared<Subscriber<T>>(topic, serializer_, std::forward<Callback>(cb));

  bool create_transport_subscription = false;

  {
    std::lock_guard<std::mutex> lock(mutex_);

    auto & subscribers = subscribers_[topic];

    create_transport_subscription = subscribers.empty();

    subscribers.push_back(subscriber);
  }

  /// Create low-level transport subscriber once
  if(create_transport_subscription)
  {
    transport_->subscribe(topic,
                          [this](const std::string & topic, const ByteBuffer & payload) { dispatch(topic, payload); });
  }

  return subscriber;
}

///------------------------------------------------------------
/// Typed queryable registration
///------------------------------------------------------------

template<typename T, typename Handler>
void Communication::registerQueryable(const std::string & topic, Handler && handler)
{
  transport_->registerQueryable(topic,
                                [this, handler](const std::string & /*topic*/) -> ByteBuffer
                                {
                                  T value = handler();
                                  return serializer_->serialize(value);
                                });
}

///------------------------------------------------------------
/// Typed query
///------------------------------------------------------------

template<typename T>
std::optional<T> Communication::query(const std::string & topic, std::chrono::milliseconds timeout)
{
  auto bytes = transport_->query(topic, timeout);
  if(!bytes)
  {
    return std::nullopt;
  }

  return serializer_->deserialize<T>(MessageTraits<T>::type, bytes->data(), bytes->size());
}

} // namespace mc_communication

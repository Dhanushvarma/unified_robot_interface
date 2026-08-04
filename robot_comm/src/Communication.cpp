#include <robot_comm/Communication.h>

#include <robot_comm/serialization/FlatbufferSerializer.h>
#include <robot_comm/serialization/ProtobufSerializer.h>

#include <mc_rtc/logging.h>

#include <stdexcept>
#include <utility>

namespace robot_comm
{

Communication::Communication(std::string name, const mc_rtc::Configuration & config, SerializationBackend backend)
: name_(std::move(name))
{
  auto parsed_config = parseConfig(config);

  switch(backend)
  {
    case SerializationBackend::Flatbuffer:
    {
      serializer_ = std::make_shared<FlatbufferSerializer>();
      break;
    }

    case SerializationBackend::Protobuf:
    {
      serializer_ = std::make_shared<ProtobufSerializer>();
      break;
    }

    default:
    {
      throw std::runtime_error("[Communication] Unsupported serialization backend");
    }
  }

  mc_rtc::log::info("[Communication] Initialized '{}' with backend '{}'", name_, parsed_config("backend"));
}

Communication::~Communication() {}

mc_rtc::Configuration Communication::parseConfig(const mc_rtc::Configuration & config)
{
  auto cfg = config;

  if(!cfg.has("backend"))
  {
    cfg.add("backend", "flatbuffer");
  }

  backendFromString(cfg("backend"));

  if(!cfg.has("protocol"))
  {
    throw std::runtime_error("[Communication] Missing required field: protocol");
  }

  auto protocol = protocolFromString(cfg("protocol"));

  const bool requires_ip = protocol != Protocol::ZENOH && protocol != Protocol::ZENOH_SHM;

  if(requires_ip && !cfg.has("ip"))
  {
    throw std::runtime_error("[Communication] Missing required field: ip");
  }

  return cfg;
}

SerializationBackend Communication::backendFromString(const std::string & backend)
{
  if(backend == "flatbuffer")
  {
    return SerializationBackend::Flatbuffer;
  }
  if(backend == "protobuf")
  {
    return SerializationBackend::Protobuf;
  }

  throw std::runtime_error("[Communication] Unknown backend: " + backend);
}

Protocol Communication::protocolFromString(const std::string & protocol)
{
  if(protocol == "tcp")
  {
    return Protocol::TCP;
  }
  if(protocol == "udp")
  {
    return Protocol::UDP;
  }
  if(protocol == "zenoh")
  {
    return Protocol::ZENOH;
  }
  if(protocol == "zenoh/shm")
  {
    return Protocol::ZENOH_SHM;
  }
  if(protocol == "shm")
  {
    return Protocol::SHM;
  }

  throw std::runtime_error("[Communication] Unknown protocol: " + protocol);
}

void Communication::dispatch(const std::string & topic, const ByteBuffer & buffer)
{
  std::vector<std::shared_ptr<SubscriberBase>> subscribers_copy;

  {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = subscribers_.find(topic);

    if(it == subscribers_.end())
    {
      return;
    }

    subscribers_copy = it->second;
  }

  for(const auto & subscriber : subscribers_copy)
  {
    subscriber->receive(buffer);
  }
}

void Communication::handleQuery(const std::string & topic, QueryHandler handler)
{
  transport_->registerQueryable(topic, std::move(handler));
}

std::optional<ByteBuffer> Communication::query(const std::string & topic,
                                               const ByteBuffer & payload,
                                               std::chrono::milliseconds timeout)
{
  return transport_->query(topic, payload, timeout);
}

void Communication::setupServer()
{
  tx_topic_ = name_ + "/command";
  rx_topic_ = name_ + "/state";

  transport_->subscribe(rx_topic_,
                        [this](const std::string & /*topic*/, const ByteBuffer & payload)
                        {
                          std::lock_guard<std::mutex> lock(rx_mutex_);
                          latest_rx_ = payload;
                        });

  mc_rtc::log::info("[Communication] '{}' configured as server (rx={}, tx={})", name_, rx_topic_, tx_topic_);
}

void Communication::setupClient()
{
  tx_topic_ = name_ + "/state";
  rx_topic_ = name_ + "/command";

  transport_->subscribe(rx_topic_,
                        [this](const std::string & /*topic*/, const ByteBuffer & payload)
                        {
                          std::lock_guard<std::mutex> lock(rx_mutex_);
                          latest_rx_ = payload;
                        });

  mc_rtc::log::info("[Communication] '{}' configured as client (rx={}, tx={})", name_, rx_topic_, tx_topic_);
}

bool Communication::send(const ByteBuffer & buffer)
{
  if(tx_topic_.empty())
  {
    mc_rtc::log::error("[Communication] '{}' send() called before setupServer/Client()", name_);
    return false;
  }
  return transport_->publish(tx_topic_, buffer);
}

std::optional<ByteBuffer> Communication::receive()
{
  if(rx_topic_.empty())
  {
    mc_rtc::log::error("[Communication] '{}' receive() called before setupServer/Client()", name_);
    return std::nullopt;
  }
  std::lock_guard<std::mutex> lock(rx_mutex_);
  return latest_rx_.value_or(ByteBuffer{});
}

} // namespace robot_comm

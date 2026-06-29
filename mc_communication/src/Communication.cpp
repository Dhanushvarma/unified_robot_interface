#include <mc_communication/Communication.h>

#include <mc_communication/serialization/FlatbufferSerializer.h>

#ifdef WITH_PROTOBUF
#  include <mc_communication/serialization/ProtobufSerializer.h>
#endif

#include <mc_rtc/logging.h>

#include <stdexcept>
#include <utility>

namespace mc_communication
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

#ifdef WITH_PROTOBUF
    case SerializationBackend::Protobuf:
    {
      serializer_ = std::make_shared<ProtobufSerializer>();
      break;
    }
#endif

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

} // namespace mc_communication

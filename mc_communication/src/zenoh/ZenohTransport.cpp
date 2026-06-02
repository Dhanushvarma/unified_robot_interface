#include <mc_communication/zenoh/ZenohTransport.h>

#include <mc_rtc/logging.h>

#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

namespace mc_communication
{

zenoh::Config ZenohTransport::configureTransport(const mc_rtc::Configuration & config)
{
  auto zenoh_config = zenoh::Config::create_default();

  const std::string protocol = config("protocol");

  if(protocol == "zenoh/shm")
  {
    zenoh_config.insert_json5("transport/shared_memory/enabled", "true");
    zenoh_config.insert_json5("mode", "\"peer\"");
  }

  if(config.has("configuration"))
  {
    const std::string path = config("configuration");

    if(!fs::exists(path))
    {
      mc_rtc::log::error_and_throw("[ZenohTransport] Configuration "
                                   "file does not exist: {}",
                                   path);
    }

    mc_rtc::log::info("[ZenohTransport] Loading "
                      "configuration: {}",
                      path);

    zenoh_config = zenoh::Config::from_file(path);
  }

  return zenoh_config;
}

ZenohTransport::ZenohTransport(const mc_rtc::Configuration & config) : TransportInterface(config)
{
  auto zenoh_config = configureTransport(config_);

  session_ = std::make_unique<zenoh::Session>(std::move(zenoh_config));
}

void ZenohTransport::start()
{
  mc_rtc::log::info("[ZenohTransport] Started");
}

void ZenohTransport::stop()
{
  subscribers_.clear();
  publishers_.clear();

  mc_rtc::log::info("[ZenohTransport] Stopped");
}

bool ZenohTransport::publish(const std::string & topic, const ByteBuffer & payload)
{
  auto it = publishers_.find(topic);

  if(it == publishers_.end())
  {
    auto pub = session_->declare_publisher(topic);

    it = publishers_.emplace(topic, std::move(pub)).first;
  }

  it->second.put(payload);

  return true;
}

void ZenohTransport::subscribe(const std::string & topic, ReceiveCallback callback)
{
  if(hasSubscriber(topic))
  {
    return;
  }

  auto subscriber = session_->declare_subscriber(
      zenoh::KeyExpr(topic),
      [callback](const zenoh::Sample & sample)
      {
        auto bytes = sample.get_payload().as_vector();

        ByteBuffer payload(bytes.begin(), bytes.end());

        callback(static_cast<std::string>(sample.get_keyexpr().as_string_view()), payload);
      },
      zenoh::closures::none);

  subscribers_.emplace(topic, std::move(subscriber));

  mc_rtc::log::info("[ZenohTransport] Subscribed to '{}'", topic);
}

bool ZenohTransport::hasSubscriber(const std::string & topic) const
{
  return subscribers_.find(topic) != subscribers_.end();
}

} // namespace mc_communication

#include <mc_communication/zenoh/ZenohTransport.h>

#include <mc_rtc/logging.h>

#include <filesystem>
#include <thread>
#include <utility>

namespace fs = std::filesystem;

namespace mc_communication
{

zenoh::Config ZenohTransport::configureTransport(const mc_rtc::Configuration & config)
{
  auto zenoh_config = zenoh::Config::create_default();

  const std::string protocol = config("protocol");

  // TODO: consider give user warning instead of modifying zenoh_config
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
  queryables_.clear();

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

void ZenohTransport::registerQueryable(const std::string & topic, QueryHandler handler)
{
  if(queryables_.count(topic))
  {
    mc_rtc::log::warning("[ZenohTransport] Queryable for '{}' already registered", topic);
    return;
  }

  auto on_query = [handler = std::move(handler), topic](const zenoh::Query & query)
  {
    ByteBuffer reply = handler(topic);
    query.reply(query.get_keyexpr(), zenoh::Bytes(reply));
  };

  auto qbl = session_->declare_queryable(
      topic, std::move(on_query), []() {}, zenoh::Session::QueryableOptions{.complete = true});

  queryables_.emplace(topic, std::move(qbl));

  mc_rtc::log::info("[ZenohTransport] Registered queryable on '{}'", topic);
}

std::optional<ByteBuffer> ZenohTransport::query(const std::string & topic, std::chrono::milliseconds timeout)
{
  auto replies = session_->get(topic, "", zenoh::channels::FifoChannel(16));

  const auto deadline = std::chrono::steady_clock::now() + timeout;

  while(std::chrono::steady_clock::now() < deadline)
  {
    auto res = replies.try_recv();

    if(std::holds_alternative<zenoh::Reply>(res))
    {
      const auto & reply = std::get<zenoh::Reply>(res);
      if(reply.is_ok())
      {
        const auto & sample = reply.get_ok();
        auto bytes = sample.get_payload().as_vector();
        return ByteBuffer(bytes.begin(), bytes.end());
      }
      mc_rtc::log::warning("[ZenohTransport] Received an error reply for '{}'", topic);
    }
    else if(std::get<zenoh::channels::RecvError>(res) == zenoh::channels::RecvError::Z_NODATA)
    {
      // No reply yet — sleep and retry
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    else
    {
      // Channel closed
      break;
    }
  }

  mc_rtc::log::warning("[ZenohTransport] Query on '{}' timed out", topic);
  return std::nullopt;
}

} // namespace mc_communication

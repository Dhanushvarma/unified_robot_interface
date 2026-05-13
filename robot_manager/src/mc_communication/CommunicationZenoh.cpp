#include <mc_communication/CommunicationZenoh.h>
#include <mc_rtc/Configuration.h>
#include <mc_rtc/logging.h>
#include "zenoh.hxx"

#include <cstring>
#include <mutex>
#include <optional>
#include <vector>

namespace mc_communication
{

/* ---- Shared helpers -------------------------------------------------------*/

static zenoh::Config configureTransport(const mc_rtc::Configuration & com_config)
{
  auto zenoh_config = zenoh::Config::create_default();
  std::string protocol = com_config("protocol");

  if(protocol == "zenoh/shm")
  {
    zenoh_config.insert_json5("transport/shared_memory/enabled", "true");
    zenoh_config.insert_json5("mode", "\"peer\"");
    return zenoh_config;
  }
  if(protocol == "zenoh/tcp")
  {
    // TODO: configure zenoh configuration for tcp
    return zenoh_config;
  }

  if(protocol == "zenoh/udp")
  {
    // TODO: configure zenoh configuration for udp
    return zenoh_config;
  }

  mc_rtc::log::error_and_throw("Unsupported protocol: {}", protocol);
}

/* ---- Server Impl ----------------------------------------------------------*/

struct CommunicationSeverZenoh::Impl
{
  std::unique_ptr<zenoh::Session> session;
  std::optional<zenoh::Queryable<void>> config_queryable;
  std::optional<zenoh::Subscriber<void>> state_sub;
  std::optional<zenoh::Publisher> command_pub;

  std::vector<uint8_t> config_cache;
  std::mutex mutex;
};

/* --- Sever -----------------------------------------------------------------*/

CommunicationSeverZenoh::CommunicationSeverZenoh() = default;

CommunicationSeverZenoh::CommunicationSeverZenoh(const std::string & name, const mc_rtc::Configuration & com_config)
: Communication(name, com_config), impl_(std::make_unique<Impl>())
{
  mc_rtc::log::success("com Zenoh constructor start");

  zenoh::Config zenoh_config = configureTransport(com_config);

  // Open session with proper config
  impl_->session = std::make_unique<zenoh::Session>(std::move(zenoh_config));

  mc_rtc::log::info("com Zenoh constructor 2");

  /* Setup config queryable*/
  std::string config_key = name + "/config";
  auto on_query = [this](const zenoh::Query & query)
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if(!impl_->config_cache.empty())
    {
      query.reply(query.get_keyexpr(), zenoh::Bytes(impl_->config_cache));
    }
  };

  impl_->config_queryable = impl_->session->declare_queryable(
      config_key, std::move(on_query), []() {}, zenoh::Session::QueryableOptions{.complete = true});

  // TODO
  std::string state_key = name + "/state";
  std::string command_key = name + "/command";

  mc_rtc::log::success("CommunicationSeverZenoh initialized with protocol: {}", com_config("protocol"));
}

CommunicationSeverZenoh::~CommunicationSeverZenoh()
{
  mc_rtc::log::info("CommunicationSeverZenoh destructor called");
}

bool CommunicationSeverZenoh::sendMessage(Communication::MessageType type, const uint8_t * data, size_t size)
{
  switch(type)
  {
    case MessageType::CONFIG:
    {
      std::lock_guard<std::mutex> lock(impl_->mutex);
      impl_->config_cache.assign(data, data + size);
      return true;
      break;
    }
    case MessageType::STATE:
    case MessageType::COMMAND:

    default:
      break;
  }

  return false;
}

bool CommunicationSeverZenoh::receiveMessage(Communication::MessageType type)
{
  switch(type)
  {
    case MessageType::CONFIG:
    case MessageType::STATE:
    case MessageType::COMMAND:

    default:
      break;
  }
  return false;
}

/* --- Client ----------------------------------------------------------------*/

struct CommunicationClientZenoh::Impl
{
  std::unique_ptr<zenoh::Session> session;
  std::optional<zenoh::Querier> config_querier;
  std::optional<zenoh::Publisher> state_pub;
  std::optional<zenoh::Subscriber<void>> command_sub;
};

CommunicationClientZenoh::CommunicationClientZenoh() = default;

CommunicationClientZenoh::CommunicationClientZenoh(const std::string & name, const mc_rtc::Configuration & com_config)
: Communication(name, com_config)
{
  mc_rtc::log::success("CommunicationClientZenoh constructor start");

  zenoh::Config zenoh_config = configureTransport(com_config);
  impl_->session = std::make_unique<zenoh::Session>(std::move(zenoh_config));

  mc_rtc::log::info("CommunicationClientZenoh constructor 1");

  mc_rtc::log::info("Setting up Zenoh communication for robot: {}", name);

  /* Setup config querier*/
  std::string config_key = name + "/config";
  impl_->config_querier = impl_->session->declare_querier(config_key, zenoh::Session::QuerierOptions{});

  // ToDo
  std::string state_key = name + "/state";
  std::string command_key = name + "/command";
  mc_rtc::log::success("CommunicationClientZenoh initialized with protocol: {}", com_config("protocol"));
}

CommunicationClientZenoh::~CommunicationClientZenoh()
{
  mc_rtc::log::info("CommunicationClientZenoh destructor called");
}

bool CommunicationClientZenoh::sendMessage(Communication::MessageType type, const uint8_t * data, size_t size)
{
  return false;
}

bool CommunicationClientZenoh::receiveMessage(Communication::MessageType type)
{

  switch(type)
  {
    case MessageType::CONFIG:
    {
      mc_rtc::log::success("CommunicationClientZenoh::receiveMessage MessageType {}", static_cast<int>(type));

      auto replies = impl_->config_querier->get("", zenoh::channels::FifoChannel(16));

      for(auto res = replies.recv(); std::holds_alternative<zenoh::Reply>(res); res = replies.recv())
      {
        const auto & reply = std::get<zenoh::Reply>(res);

        if(reply.is_ok())
        {
          const auto & sample = reply.get_ok();
          const auto & payload = sample.get_payload();

          mc_rtc::Configuration received_config;
          Communication::deserialize(payload.as_vector().data(), received_config);

          this->updateLatestConfig(received_config);

          return true;
        }
        mc_rtc::log::error("Received a bad query reply for CONFIG");
      }

      return false;
    }

    case MessageType::STATE:
    case MessageType::COMMAND:

    default:
      break;
  }

  return false;
}

} // namespace mc_communication

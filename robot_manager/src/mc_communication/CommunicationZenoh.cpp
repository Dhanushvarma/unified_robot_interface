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

static zenoh::Config configureTransport(const mc_rtc::Configuration & mc_config, const std::string & com_config_path)
{
  std::string protocol = mc_config("protocol");

  if(!com_config_path.empty())
  {
    auto zenoh_config = zenoh::Config::from_file(com_config_path);
    if(protocol == "zenoh/shm")
    {
      if(zenoh_config.get("transport/shared_memory/enabled") != "true")
      {
        mc_rtc::log::error_and_throw(
            "Using zenoh/shm but transport/shared_memory is not enabled\n"
            "Consider adding 'enabled: true' to zenoh configuration section 'transport/shared_memory'\n"
            "For more defails, check https://github.com/eclipse-zenoh/zenoh/blob/main/DEFAULT_CONFIG.json5");
      }
    }
    return zenoh_config;
  }

  auto zenoh_config = zenoh::Config::create_default();

  // TODO: include a more robust default zenoh config
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

  std::mutex mutex;

  std::vector<uint8_t> config_cache;

  std::vector<uint8_t> state_cache;
  uint64_t state_seq = 0;
  uint64_t state_seq_consumed = 0;
};

/* --- Sever -----------------------------------------------------------------*/

CommunicationSeverZenoh::CommunicationSeverZenoh() = default;

CommunicationSeverZenoh::CommunicationSeverZenoh(const std::string & name,
                                                 const mc_rtc::Configuration & mc_config,
                                                 const std::string & com_config_path)
: Communication(name, mc_config), impl_(std::make_unique<Impl>())
{
  mc_rtc::log::success("com Zenoh constructor start");

  zenoh::Config zenoh_config = configureTransport(mc_config, com_config_path);
  impl_->session = std::make_unique<zenoh::Session>(std::move(zenoh_config));

  mc_rtc::log::info("com Zenoh constructor 2");

  /* Setup config queryable */
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

  /* Setup state subscriber */
  std::string state_key = name + "/state";
  auto state_handler = [this](const zenoh::Sample & sample)
  {
    const auto & payload = sample.get_payload();
    auto bytes = payload.as_vector();
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->state_cache.assign(bytes.begin(), bytes.end());
    ++impl_->state_seq;
  };

  impl_->state_sub =
      impl_->session->declare_subscriber(zenoh::KeyExpr(state_key), std::move(state_handler), zenoh::closures::none);

  /* Setup command publisher */
  std::string command_key = name + "/command";
  impl_->command_pub = impl_->session->declare_publisher(command_key);

  mc_rtc::log::success("CommunicationSeverZenoh initialized with protocol: {}", mc_config("protocol"));
}

CommunicationSeverZenoh::~CommunicationSeverZenoh()
{
  mc_rtc::log::info("CommunicationSeverZenoh destructor called");
}

bool CommunicationSeverZenoh::sendMessage(Communication::MessageType type, const uint8_t * data, size_t size)
{
  if(data == nullptr || size == 0)
  {
    return false;
  }

  switch(type)
  {
    case MessageType::CONFIG:
    {
      std::lock_guard<std::mutex> lock(impl_->mutex);
      impl_->config_cache.assign(data, data + size);
      return true;
    }

    case MessageType::COMMAND:
    {
      if(!impl_ || !impl_->command_pub)
      {
        return false;
      }

      std::vector<uint8_t> payload(data, data + size);
      impl_->command_pub->put(zenoh::Bytes(std::move(payload)));
      return true;
    }

    default:
      break;
  }

  return false;
}

bool CommunicationSeverZenoh::receiveMessage(Communication::MessageType type)
{
  switch(type)
  {
    case MessageType::STATE:
    {
      std::vector<uint8_t> local;
      {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if(impl_->state_seq == impl_->state_seq_consumed)
        {
          return false;
        }
        impl_->state_seq_consumed = impl_->state_seq;
        local = impl_->state_cache;
      }

      auto s = Communication::deserializeState(local.data(), local.size());
      if(!s)
      {
        mc_rtc::log::error("STATE payload failed to deserialize");
        return false;
      }

      this->updateLatestState(*s);
      return true;
    }

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

  std::mutex mutex;

  std::vector<uint8_t> command_cache;
  uint64_t command_seq = 0;
  uint64_t command_seq_consumed = 0;
};

CommunicationClientZenoh::CommunicationClientZenoh() = default;

CommunicationClientZenoh::CommunicationClientZenoh(const std::string & name,
                                                   const mc_rtc::Configuration & mc_config,
                                                   const std::string & com_config_path)
: Communication(name, mc_config), impl_(std::make_unique<Impl>())
{
  mc_rtc::log::success("CommunicationClientZenoh constructor start");

  zenoh::Config zenoh_config = configureTransport(mc_config, com_config_path);
  impl_->session = std::make_unique<zenoh::Session>(std::move(zenoh_config));

  mc_rtc::log::info("CommunicationClientZenoh constructor 1");

  mc_rtc::log::info("Setting up Zenoh communication for robot: {}", name);

  /* Setup config querier */
  std::string config_key = name + "/config";
  impl_->config_querier = impl_->session->declare_querier(config_key, zenoh::Session::QuerierOptions{});

  /* Setup state publisher */
  std::string state_key = name + "/state";
  impl_->state_pub = impl_->session->declare_publisher(state_key);

  /* Setup command subscriber */
  std::string command_key = name + "/command";

  auto command_handler = [this](const zenoh::Sample & sample)
  {
    const auto & payload = sample.get_payload();
    auto bytes = payload.as_vector();
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->command_cache.assign(bytes.begin(), bytes.end());
    ++impl_->command_seq;
  };

  impl_->command_sub =
      impl_->session->declare_subscriber(command_key, std::move(command_handler), zenoh::closures::none);

  mc_rtc::log::success("CommunicationClientZenoh initialized with protocol: {}", mc_config("protocol"));
}

CommunicationClientZenoh::~CommunicationClientZenoh()
{
  mc_rtc::log::info("CommunicationClientZenoh destructor called");
}

bool CommunicationClientZenoh::sendMessage(Communication::MessageType type, const uint8_t * data, size_t size)
{
  if(data == nullptr || size == 0)
  {
    return false;
  }

  switch(type)
  {
    case MessageType::STATE:
    {
      if(!impl_ || !impl_->state_pub)
      {
        return false;
      }

      std::vector<uint8_t> payload(data, data + size);
      impl_->state_pub->put(zenoh::Bytes(std::move(payload)));
      return true;
    }

    default:
      break;
  }

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
          const auto & data = sample.get_payload().as_vector();

          auto c = Communication::deserializeConfig(data.data(), data.size());
          if(!c)
          {
            mc_rtc::log::error("Failed to deserialize configuration message");
            return false;
          }
          this->updateLatestConfig(*c);

          return true;
        }
        mc_rtc::log::error("Received a bad query reply for CONFIG");
      }

      return false;
    }

    case MessageType::COMMAND:
    {
      std::vector<uint8_t> local;
      {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if(impl_->command_seq == impl_->command_seq_consumed)
        {
          return false;
        }
        impl_->command_seq_consumed = impl_->command_seq;
        local = impl_->command_cache;
      }

      auto c = Communication::deserializeCommand(local.data(), local.size());
      if(!c)
      {
        mc_rtc::log::error("command payload failed to deserialize");
        return false;
      }

      // Store it somewhere useful
      this->updateLatestCommand(*c);
      return true;
    }

    default:
      break;
  }

  return false;
}

} // namespace mc_communication

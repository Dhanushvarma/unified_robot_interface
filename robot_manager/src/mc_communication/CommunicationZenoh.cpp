#include <mc_communication/CommunicationZenoh.h>
#include <mc_rtc/Configuration.h>

#include <cstring>

namespace mc_communication
{

/* --- Sever -----------------------------------------------------------------*/

CommunicationSeverZenoh::CommunicationSeverZenoh(const std::string & name, const mc_rtc::Configuration & com_config)
: Communication(name, com_config)
{
  mc_rtc::log::success("com Zenoh constructor start");

  zenoh::Config zenoh_config = configureTransport(com_config);

  // Open session with proper config
  session_ = std::make_unique<zenoh::Session>(std::move(zenoh_config));

  mc_rtc::log::info("com Zenoh constructor 2");

  setupTransport(com_config);

  mc_rtc::log::success("CommunicationSeverZenoh initialized with protocol: {}", com_config("protocol"));
}

CommunicationSeverZenoh::~CommunicationSeverZenoh()
{
  mc_rtc::log::info("CommunicationSeverZenoh destructor called");
}

zenoh::Config CommunicationSeverZenoh::configureTransport(const mc_rtc::Configuration & com_config)
{

  auto zenoh_config = zenoh::Config::create_default();

  std::string protocol = com_config("protocol");

  if(protocol == "zenoh/shm")
  {
    mc_rtc::log::info("com Zenoh configureShm");
    zenoh_config.insert_json5("transport/shared_memory/enabled", "true");
    zenoh_config.insert_json5("mode", "\"peer\"");

    return zenoh_config;
  }

  if(protocol == "zenoh/tcp")
  {
    // ToDo: configure zenoh configuration for tcp
    return zenoh_config;
  }

  if(protocol == "zenoh/udp")
  {
    // ToDo: configure zenoh configuration for udp
    return zenoh_config;
  }

  mc_rtc::log::error_and_throw("Unsupported protocol: {}", protocol);
}

void CommunicationSeverZenoh::setupTransport(const mc_rtc::Configuration & com_config)
{
  std::string robot_name = Communication::name();

  mc_rtc::log::info("Setting up Zenoh communication for robot: {}", robot_name);

  /* Setup config queryable*/
  std::string config_key = robot_name + "/config";
  auto on_query = [this](const zenoh::Query & query)
  {
    std::lock_guard<std::mutex> lock(this->mutex_);
    if(!this->config_cache_.empty())
    {
      query.reply(query.get_keyexpr(), zenoh::Bytes(this->config_cache_));
    }
  };

  config_queryable_ = session_->declare_queryable(
      config_key, std::move(on_query), []() {}, zenoh::Session::QueryableOptions{.complete = true});

  // ToDo
  std::string state_key = robot_name + "/state";
  std::string command_key = robot_name + "/command";

  mc_rtc::log::info("Subscribers created");
}

bool CommunicationSeverZenoh::sendMessage(Communication::MessageType type, const uint8_t * data, size_t size)
{
  switch(type)
  {
    case MessageType::CONFIG:
    {
      std::lock_guard<std::mutex> lock(mutex_);
      config_cache_.assign(data, data + size);
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

CommunicationClientZenoh::CommunicationClientZenoh(const std::string & name, const mc_rtc::Configuration & com_config)
: Communication(name, com_config)
{
  mc_rtc::log::success("CommunicationClientZenoh constructor start");

  zenoh::Config zenoh_config = configureTransport(com_config);
  session_ = std::make_unique<zenoh::Session>(std::move(zenoh_config));

  mc_rtc::log::info("CommunicationClientZenoh constructor 1");

  setupTransport(com_config);

  mc_rtc::log::success("CommunicationClientZenoh initialized with protocol: {}", com_config("protocol"));
}

CommunicationClientZenoh::~CommunicationClientZenoh()
{
  mc_rtc::log::info("CommunicationClientZenoh destructor called");
}

zenoh::Config CommunicationClientZenoh::configureTransport(const mc_rtc::Configuration & com_config)
{

  auto zenoh_config = zenoh::Config::create_default();

  std::string protocol = com_config("protocol");

  if(protocol == "zenoh/shm")
  {
    mc_rtc::log::info("com Zenoh configureShm");
    zenoh_config.insert_json5("transport/shared_memory/enabled", "true");
    zenoh_config.insert_json5("mode", "\"peer\"");

    return zenoh_config;
  }

  if(protocol == "zenoh/tcp")
  {
    // ToDo: configure zenoh configuration for tcp
    return zenoh_config;
  }

  if(protocol == "zenoh/udp")
  {
    // ToDo: configure zenoh configuration for udp
    return zenoh_config;
  }

  mc_rtc::log::error_and_throw("Unsupported protocol: {}", protocol);
}

void CommunicationClientZenoh::setupTransport(const mc_rtc::Configuration & com_config)
{
  std::string robot_name = Communication::name();

  mc_rtc::log::info("Setting up Zenoh communication for robot: {}", robot_name);

  /* Setup config querier*/
  std::string config_key = robot_name + "/config";
  config_querier_ = session_->declare_querier(config_key, zenoh::Session::QuerierOptions{});

  // ToDo
  std::string state_key = robot_name + "/state";
  std::string command_key = robot_name + "/command";

  mc_rtc::log::info("Subscribers created");
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

      auto replies = config_querier_->get("", zenoh::channels::FifoChannel(16));

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

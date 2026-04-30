#include <mc_communication/CommunicationZenoh.h>
#include <mc_rtc/Configuration.h>

#include <cstring>

namespace mc_communication
{

CommunicationSeverZenoh::CommunicationSeverZenoh(const mc_rtc::Configuration & com_config) : Communication(com_config)
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

  std::string robot_name;

  if(com_config.has("name"))
  {
    robot_name = static_cast<std::string>(com_config("name"));
    mc_rtc::log::info("Setting up publishers and subscribers for robot: {}", robot_name);
  }
  else
  {
    // mc_rtc::log::info("NAME EXTRACTED {}", RoboterInterface::name_);
  }

  // Setup publishers
  std::string config_key = robot_name + "/config";
  std::string state_key = robot_name + "/state";
  std::string command_key = robot_name + "/command";

  mc_rtc::log::info("Subscribers created");
}

bool CommunicationSeverZenoh::sendMessage(const uint8_t * data, size_t size)
{
  // ToDo: sendmessage using zenoh

  // mc_rtc::log::info("Sending config message");
  // auto data = serialize(message);
  // config_pub_->put(zenoh::Bytes(data));

  return true;
}

flatbuffers::FlatBufferBuilder CommunicationSeverZenoh::serialize(const mc_rtc::Configuration & config)
{
  flatbuffers::FlatBufferBuilder builder;

  // First, lets serialize some weapons for the Monster: A 'sword' and an 'axe'.
  auto config_json = builder.CreateString(config.dump());

  // Use the `CreateWeapon` shortcut to create Weapons with all fields set.
  auto message = CreateMessageConfig(builder, config_json);
  builder.Finish(message);

  return builder;
}

} // namespace mc_communication

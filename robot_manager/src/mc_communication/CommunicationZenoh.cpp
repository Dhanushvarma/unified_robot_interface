#include <mc_communication/CommunicationZenoh.h>
#include <mc_rtc/Configuration.h>

#include <cstring>

namespace mc_communication
{

CommunicationZenoh::CommunicationZenoh(const mc_rtc::Configuration & com_config) : Communication(com_config)
{
  mc_rtc::log::success("com Zenoh constructor start");

  // Configure Zenoh transport based on protocol
  auto zenoh_config = zenoh::Config::create_default();

  mc_rtc::log::info("com Zenoh constructor 1");

  configureTransport(com_config, zenoh_config);

  // Open session with proper config
  session_ = std::make_unique<zenoh::Session>(std::move(zenoh_config));

  mc_rtc::log::info("com Zenoh constructor 2");

  setupPubSub(com_config);

  mc_rtc::log::success("CommunicationZenoh initialized with protocol: {}", com_config("protocol"));
}

CommunicationZenoh::~CommunicationZenoh()
{
  mc_rtc::log::info("CommunicationZenoh destructor called");
}

void CommunicationZenoh::configureTransport(const mc_rtc::Configuration & com_config, zenoh::Config & zenoh_config)
{
  std::string protocol = com_config("protocol");

  if(protocol == "zenoh/shm")
  {
    mc_rtc::log::info("com Zenoh configureShm");
    zenoh_config.insert_json5("transport/shared_memory/enabled", "true");
    zenoh_config.insert_json5("mode", "\"peer\"");
  }
  else if(protocol == "zenoh/tcp")
  {
    // ToDo: configure zenoh configuration for tcp
  }
  else if(protocol == "zenoh/udp")
  {
    // ToDo: configure zenoh configuration for udp
  }
  else
  {
    mc_rtc::log::error_and_throw("Unsupported protocol: {}", protocol);
  }
}

void CommunicationZenoh::setupPubSub(const mc_rtc::Configuration & com_config)
{

  // if(com_config.has("name"))
  // {
  //   mc_rtc::log::info("CommunicationZenoh constructor subscriber");

  //   // create subscriber then wait for first message
  //   std::string config_key = com_config("name");

  //   auto data_handler = [](const zenoh::Sample & sample)
  //   {
  //     std::cout << ">> [Subscriber] Received "
  //               << " ('" << sample.get_keyexpr().as_string_view() << "' : '" << sample.get_payload().as_string()
  //               << "')";

  //     auto attachment = sample.get_attachment();
  //     if(attachment.has_value())
  //     {
  //       std::cout << "  (" << attachment->get().as_string() << ")";
  //     }
  //     std::cout << std::endl;
  //   };

  //   mc_rtc::log::info("CommunicationZenoh constructor subscriber 1");

  //   config_sub_ = session_->declare_subscriber(zenoh::KeyExpr(config_key), data_handler, zenoh::closures::none);
  //   mc_rtc::log::info("Created subsriber with key {}", config_sub_->get_keyexpr().as_string_view());
  // }
  // else
  // {
  //   mc_rtc::log::info("CommunicationZenoh constructor publisher");

  //   // create publisher and subscriber
  //   std::string config_key = "manager/" + name + "/config";
  //   config_pub_ = session_->declare_publisher(zenoh::KeyExpr(config_key));
  //   mc_rtc::log::info("Created publisher with key {}", config_pub_->get_keyexpr().as_string_view());
  //   config_pub_->put("Simple from publisher.put!");
  // }

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

bool CommunicationZenoh::sendMessage(const std::string & message)
{
  // mc_rtc::log::info("Sending config message");
  // auto data = serialize(message);
  // config_pub_->put(zenoh::Bytes(data));
  return true;
}

bool CommunicationZenoh::receiveMessage(std::string & message)
{
  // std::lock_guard<std::mutex> lock(mutex_);
  // if(latest_config_)
  // {
  //   message = *latest_config_;
  //   return true;
  // }
  return false;
}

} // namespace mc_communication

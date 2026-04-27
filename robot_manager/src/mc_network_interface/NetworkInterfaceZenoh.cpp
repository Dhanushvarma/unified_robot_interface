#include <mc_rtc/Configuration.h>
#include <mc_network_interface/NetworkInterfaceZenoh.h>

#include <cstring>

namespace mc_network
{

NetworkInterfaceZenoh::NetworkInterfaceZenoh(const mc_rtc::Configuration & network_config)
: NetworkInterface(network_config)
{
  mc_rtc::log::success("NetworkInterfaceZenoh constructor start");

  std::string protocol = network_config("protocol");

  mc_rtc::log::info("NetworkInterfaceZenoh constructor 1");

  // Configure Zenoh transport based on protocol
  auto zenoh_config = zenoh::Config::create_default();

  mc_rtc::log::info("NetworkInterfaceZenoh constructor 2");

  if(protocol == "zenoh/shm")
  {
    configureShm(zenoh_config);
  }
  else if(protocol == "zenoh/tcp")
  {
    configureTcp(zenoh_config);
  }
  else if(protocol == "zenoh/udp")
  {
    configureUdp(zenoh_config);
  }
  else
  {
    mc_rtc::log::error_and_throw("Unsupported protocol: {}", protocol);
  }

  // Open session with proper config
  session_ = std::make_unique<zenoh::Session>(std::move(zenoh_config));

  mc_rtc::log::info("NetworkInterfaceZenoh constructor 3");

  setupPubSub(network_config);

  mc_rtc::log::success("NetworkInterfaceZenoh initialized with protocol: {}", protocol);
}

NetworkInterfaceZenoh::~NetworkInterfaceZenoh()
{
  mc_rtc::log::info("NetworkInterfaceZenoh destructor called");
  // Zenoh handles cleanup automatically via RAII
}

void NetworkInterfaceZenoh::configureShm(zenoh::Config & zenoh_config)
{
  mc_rtc::log::info("network Zenoh configureShm");

  zenoh_config.insert_json5("transport/shared_memory/enabled", "true");
  zenoh_config.insert_json5("mode", "\"peer\"");
}

void NetworkInterfaceZenoh::configureTcp(zenoh::Config & zenoh_config)
{
  // const std::string & ip_addr = ip();
  // uint16_t zenoh_port = NetworkInterface::port();

  // std::string listen_endpoint = "tcp/" + ip_addr + ":" + std::to_string(zenoh_port);

  // mc_rtc::log::info("Configuring TCP transport: {}", listen_endpoint);

  // std::string listen_json = "[\"" + listen_endpoint + "\"]";
  // zenoh_config.insert_json5("listen", listen_json);
  // zenoh_config.insert_json5("mode", "\"peer\"");
}

void NetworkInterfaceZenoh::configureUdp(zenoh::Config & zenoh_config)
{
  // const std::string & multicast_ip = ip();
  // uint16_t udp_port = NetworkInterface::port();

  // std::string multicast_endpoint = "udp/" + multicast_ip + ":" + std::to_string(udp_port);

  // mc_rtc::log::info("Configuring UDP multicast: {}", multicast_endpoint);

  // std::string listen_json = "[\"" + multicast_endpoint + "\"]";
  // zenoh_config.insert_json5("listen", listen_json);
  // zenoh_config.insert_json5("mode", "\"peer\"");
}

void NetworkInterfaceZenoh::setupPubSub(const mc_rtc::Configuration & network_config)
{

  // if(network_config.has("name"))
  // {
  //   mc_rtc::log::info("NetworkInterfaceZenoh constructor subscriber");

  //   // create subscriber then wait for first message
  //   std::string config_key = network_config("name");

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

  //   mc_rtc::log::info("NetworkInterfaceZenoh constructor subscriber 1");

  //   config_sub_ = session_->declare_subscriber(zenoh::KeyExpr(config_key), data_handler, zenoh::closures::none);
  //   mc_rtc::log::info("Created subsriber with key {}", config_sub_->get_keyexpr().as_string_view());
  // }
  // else
  // {
  //   mc_rtc::log::info("NetworkInterfaceZenoh constructor publisher");

  //   // create publisher and subscriber
  //   std::string config_key = "manager/" + name + "/config";
  //   config_pub_ = session_->declare_publisher(zenoh::KeyExpr(config_key));
  //   mc_rtc::log::info("Created publisher with key {}", config_pub_->get_keyexpr().as_string_view());
  //   config_pub_->put("Simple from publisher.put!");
  // }

  std::string robot_name;

  if(network_config.has("name"))
  {
    robot_name = static_cast<std::string>(network_config("name"));
    mc_rtc::log::info("Setting up publishers and subscribers for robot: {}", robot_name);
  }
  else
  {
    // mc_rtc::log::info("NAME EXTRACTED {}", RoboterInterface::name_);
  }

  // // Setup publishers
  std::string config_key = robot_name + "/config";
  std::string state_key = robot_name + "/state";
  std::string command_key = robot_name + "/command";

  config_pub_ = session_->declare_publisher(zenoh::KeyExpr(config_key));
  state_pub_ = session_->declare_publisher(zenoh::KeyExpr(state_key));
  command_pub_ = session_->declare_publisher(zenoh::KeyExpr(command_key));

  // Setup subscribers with callbacks
  auto config_handler = [this](const zenoh::Sample & sample)
  {
    // std::lock_guard<std::mutex> lock(mutex_);
    MessageConfig message;
    auto payload = sample.get_payload().as_vector();
    if(deserialize(payload, message))
    {
      latest_config_ = message;
      mc_rtc::log::info("Received config message");
    }
  };

  config_sub_ = session_->declare_subscriber(zenoh::KeyExpr(config_key), config_handler, zenoh::closures::none);

  // state_sub_ = std::make_unique<zenoh::Subscriber<void>>(session_->declare_subscriber(
  //     zenoh::KeyExpr(state_key),
  //     [this](const zenoh::Sample & sample)
  //     {
  //       std::lock_guard<std::mutex> lock(mutex_);
  //       MessageState message;
  //       auto payload = sample.get_payload().as_vector();
  //       if(deserialize(payload, message))
  //       {
  //         latest_state_ = message;
  //       }
  //     },
  //     zenoh::closures::none));

  // command_sub_ = std::make_unique<zenoh::Subscriber<void>>(session_->declare_subscriber(
  //     zenoh::KeyExpr(command_key),
  //     [this](const zenoh::Sample & sample)
  //     {
  //       std::lock_guard<std::mutex> lock(mutex_);
  //       MessageCommand message;
  //       auto payload = sample.get_payload().as_vector();
  //       if(deserialize(payload, message))
  //       {
  //         latest_command_ = message;
  //       }
  //     },
  //     zenoh::closures::none));

  mc_rtc::log::info("Subscribers created");
}

// Serialization implementations
std::vector<uint8_t> NetworkInterfaceZenoh::serialize(const MessageConfig & message)
{
  // Simple serialization: name_size + name + config_size + config + read flag
  std::string config_str = message.config.dump();

  size_t name_len = message.name.size();
  size_t config_len = config_str.size();

  std::vector<uint8_t> buffer;
  buffer.reserve(1 + name_len + 2 + config_len + 1);

  // Name size (1 byte)
  buffer.push_back(static_cast<uint8_t>(name_len));
  // Name data
  buffer.insert(buffer.end(), message.name.begin(), message.name.end());
  // Config size (2 bytes)
  buffer.push_back(static_cast<uint8_t>(config_len & 0xFF));
  buffer.push_back(static_cast<uint8_t>((config_len >> 8) & 0xFF));
  // Config data
  buffer.insert(buffer.end(), config_str.begin(), config_str.end());
  // Read flag
  buffer.push_back(message.read ? 1 : 0);

  return buffer;
}

std::vector<uint8_t> NetworkInterfaceZenoh::serialize(const MessageState & message)
{
  size_t size = message.state.size();
  std::vector<uint8_t> buffer;
  buffer.reserve(1 + size * sizeof(double));

  // Size (1 byte)
  buffer.push_back(static_cast<uint8_t>(size));
  // State data
  const uint8_t * data_ptr = reinterpret_cast<const uint8_t *>(message.state.data());
  buffer.insert(buffer.end(), data_ptr, data_ptr + size * sizeof(double));

  return buffer;
}

std::vector<uint8_t> NetworkInterfaceZenoh::serialize(const MessageCommand & message)
{
  size_t size = message.command.size();
  std::vector<uint8_t> buffer;
  buffer.reserve(1 + size * sizeof(double));

  // Size (1 byte)
  buffer.push_back(static_cast<uint8_t>(size));
  // Command data
  const uint8_t * data_ptr = reinterpret_cast<const uint8_t *>(message.command.data());
  buffer.insert(buffer.end(), data_ptr, data_ptr + size * sizeof(double));

  return buffer;
}

bool NetworkInterfaceZenoh::deserialize(const std::vector<uint8_t> & data, MessageConfig & message)
{
  if(data.empty()) return false;

  size_t pos = 0;

  // Name size
  uint8_t name_len = data[pos++];
  if(pos + name_len > data.size()) return false;

  // Name data
  message.name = std::string(data.begin() + pos, data.begin() + pos + name_len);
  pos += name_len;

  // Config size
  if(pos + 2 > data.size()) return false;
  uint16_t config_len = data[pos] | (data[pos + 1] << 8);
  pos += 2;

  // Config data
  if(pos + config_len > data.size()) return false;
  std::string config_str(data.begin() + pos, data.begin() + pos + config_len);
  pos += config_len;

  message.config = mc_rtc::Configuration::fromData(config_str);

  // Read flag
  if(pos < data.size())
  {
    message.read = (data[pos] != 0);
  }

  return true;
}

bool NetworkInterfaceZenoh::deserialize(const std::vector<uint8_t> & data, MessageState & message)
{
  if(data.empty()) return false;

  uint8_t size = data[0];
  if(data.size() < 1 + size * sizeof(double)) return false;

  message.state.resize(size);
  std::memcpy(message.state.data(), data.data() + 1, size * sizeof(double));

  return true;
}

bool NetworkInterfaceZenoh::deserialize(const std::vector<uint8_t> & data, MessageCommand & message)
{
  if(data.empty()) return false;

  uint8_t size = data[0];
  if(data.size() < 1 + size * sizeof(double)) return false;

  message.command.resize(size);
  std::memcpy(message.command.data(), data.data() + 1, size * sizeof(double));

  return true;
}

bool NetworkInterfaceZenoh::sendMessage(const std::string & message)
{
  // mc_rtc::log::info("Sending config message");
  // auto data = serialize(message);
  // config_pub_->put(zenoh::Bytes(data));
  return true;
}

bool NetworkInterfaceZenoh::receiveMessage(std::string & message)
{
  // std::lock_guard<std::mutex> lock(mutex_);
  // if(latest_config_)
  // {
  //   message = *latest_config_;
  //   return true;
  // }
  return false;
}

} // namespace mc_network

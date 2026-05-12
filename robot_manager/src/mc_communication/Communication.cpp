#include <mc_communication/Communication.h>

namespace mc_communication
{

Communication::Communication()
: Communication(mc_rtc::Configuration("../etc/communication.yaml")("name"),
                mc_rtc::Configuration("../etc/communication.yaml")) {};

Communication::Communication(std::string name, const mc_rtc::Configuration & com_config)
: name_(std::move(name)), ip_(com_config("ip")), port_(com_config("port")) {};

flatbuffers::FlatBufferBuilder Communication::serialize(const mc_rtc::Configuration & config)
{
  flatbuffers::FlatBufferBuilder builder;

  // First, lets serialize some weapons for the Monster: A 'sword' and an 'axe'.
  auto config_json = builder.CreateString(config.dump());

  // Use the `CreateWeapon` shortcut to create Weapons with all fields set.
  auto message = CreateMessageConfig(builder, config_json);
  builder.Finish(message);

  return builder;
}

void Communication::deserialize(const uint8_t * data, mc_rtc::Configuration & config)
{
  const auto * message = flatbuffers::GetRoot<MessageConfig>(data);

  if(message != nullptr && message->config() != nullptr)
  {
    config = mc_rtc::Configuration::fromData(message->config()->c_str());
  }
}

} // namespace mc_communication

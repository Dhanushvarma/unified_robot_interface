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

  auto config_json = builder.CreateString(config.dump());

  auto message = CreateMessageConfig(builder, config_json);
  builder.Finish(message);

  return builder;
}

flatbuffers::DetachedBuffer Communication::serializeState(const State & message_state)
{
  flatbuffers::FlatBufferBuilder builder;

  auto position = builder.CreateVector(message_state.position);
  auto velocity = builder.CreateVector(message_state.velocity);
  auto torque = builder.CreateVector(message_state.torque);

  auto message = CreateMessageState(builder, position, velocity, torque);
  builder.Finish(message);

  return builder.Release();
}

void Communication::deserialize(const uint8_t * data, mc_rtc::Configuration & message_config)
{
  const auto * message = flatbuffers::GetRoot<MessageConfig>(data);

  if(message != nullptr && message->config() != nullptr)
  {
    message_config = mc_rtc::Configuration::fromData(message->config()->c_str());
  }
}

std::optional<State> Communication::deserializeState(const uint8_t * data, size_t size)
{
  flatbuffers::Verifier verifier(data, size);
  if(!verifier.VerifyBuffer<MessageState>())
  {
    return std::nullopt;
  }

  const auto * msg = flatbuffers::GetRoot<MessageState>(data);

  State state;

  if(const auto * p = msg->position())
  {
    state.position.assign(p->begin(), p->end());
  }
  if(const auto * v = msg->velocity())
  {
    state.velocity.assign(v->begin(), v->end());
  }
  if(const auto * t = msg->torque())
  {
    state.torque.assign(t->begin(), t->end());
  }

  return state;
}

} // namespace mc_communication

#include <mc_communication/Communication.h>

namespace mc_communication
{

Communication::Communication()
: Communication(mc_rtc::Configuration("../etc/communication.yaml")("name"),
                mc_rtc::Configuration("../etc/communication.yaml")) {};

Communication::Communication(std::string name, const mc_rtc::Configuration & mc_config)
: name_(std::move(name)), ip_(mc_config.has("ip") ? static_cast<std::string>(mc_config("ip")) : ""),
  port_(mc_config.has("port") ? static_cast<std::uint16_t>(mc_config("port")) : 0)
{
  if(static_cast<std::string>(mc_config("protocol")) != "zenoh/shm")
  {
    mc_rtc::log::error_and_throw("ip and port are required for {} protocol", mc_config("protocol"));
  }
}

flatbuffers::DetachedBuffer Communication::serializeConfig(const mc_rtc::Configuration & config)
{
  flatbuffers::FlatBufferBuilder builder;

  auto config_json = builder.CreateString(config.dump());

  auto message = CreateMessageConfig(builder, config_json);
  builder.Finish(message);

  return builder.Release();
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

flatbuffers::DetachedBuffer Communication::serializeCommand(const Command & message_command)
{
  flatbuffers::FlatBufferBuilder builder;

  auto position = builder.CreateVector(message_command.position);
  auto velocity = builder.CreateVector(message_command.velocity);
  auto torque = builder.CreateVector(message_command.torque);

  auto message = CreateMessageCommand(builder, message_command.kp, message_command.kd, position, velocity, torque);
  builder.Finish(message);

  return builder.Release();
}

std::optional<mc_rtc::Configuration> Communication::deserializeConfig(const uint8_t * data, size_t size)
{
  flatbuffers::Verifier verifier(data, size);
  if(!verifier.VerifyBuffer<MessageConfig>())
  {
    return std::nullopt;
  }

  const auto * message = flatbuffers::GetRoot<MessageConfig>(data);
  if(message != nullptr && message->config() != nullptr)
  {
    return mc_rtc::Configuration::fromData(message->config()->c_str());
  }

  return nullptr;
}

std::optional<State> Communication::deserializeState(const uint8_t * data, size_t size)
{
  flatbuffers::Verifier verifier(data, size);
  if(!verifier.VerifyBuffer<MessageState>())
  {
    return std::nullopt;
  }

  const auto * message = flatbuffers::GetRoot<MessageState>(data);

  State state;

  if(const auto * p = message->position())
  {
    state.position.assign(p->begin(), p->end());
  }
  if(const auto * v = message->velocity())
  {
    state.velocity.assign(v->begin(), v->end());
  }
  if(const auto * t = message->torque())
  {
    state.torque.assign(t->begin(), t->end());
  }

  return state;
}

std::optional<Command> Communication::deserializeCommand(const uint8_t * data, size_t size)
{
  flatbuffers::Verifier verifier(data, size);
  if(!verifier.VerifyBuffer<MessageCommand>())
  {
    return std::nullopt;
  }

  const auto * message = flatbuffers::GetRoot<MessageCommand>(data);

  Command command;
  command.kp = message->kp();
  command.kd = message->kd();
  if(const auto * p = message->position())
  {
    command.position.assign(p->begin(), p->end());
  }
  if(const auto * v = message->velocity())
  {
    command.velocity.assign(v->begin(), v->end());
  }
  if(const auto * t = message->torque())
  {
    command.torque.assign(t->begin(), t->end());
  }

  return command;
}

} // namespace mc_communication

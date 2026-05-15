#include <mc_communication/Communication.h>

namespace mc_communication
{

Communication::Communication()
: Communication(mc_rtc::Configuration("../etc/communication.yaml")("name"),
                mc_rtc::Configuration("../etc/communication.yaml")) {};

Communication::Communication(std::string name, const mc_rtc::Configuration & com_config)
: name_(std::move(name)), ip_(com_config("ip")), port_(com_config("port")) {};

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

// TODO: delete logging
void Communication::dumpLog(const std::string & filename) const
{
  std::ofstream file(filename);
  if(!file.is_open())
  {
    mc_rtc::log::error("Failed to open log file: {}", filename);
    return;
  }

  file << "{\n";

  // ── States ──
  file << "  \"states\": [\n";
  for(size_t i = 0; i < states_.size(); ++i)
  {
    const auto & s = states_[i];
    file << "    {\n";
    file << "      \"index\": " << i << ",\n";

    file << "      \"position\": [";
    for(size_t j = 0; j < s.position.size(); ++j)
    {
      file << s.position[j];
      if(j + 1 < s.position.size())
      {
        file << ", ";
      }
    }
    file << "],\n";

    file << "      \"velocity\": [";
    for(size_t j = 0; j < s.velocity.size(); ++j)
    {
      file << s.velocity[j];
      if(j + 1 < s.velocity.size())
      {
        file << ", ";
      }
    }
    file << "],\n";

    file << "      \"torque\": [";
    for(size_t j = 0; j < s.torque.size(); ++j)
    {
      file << s.torque[j];
      if(j + 1 < s.torque.size())
      {
        file << ", ";
      }
    }
    file << "]\n";

    file << "    }";
    if(i + 1 < states_.size())
    {
      file << ",";
    }
    file << "\n";
  }
  file << "  ],\n";

  // ── Commands ──
  file << "  \"commands\": [\n";
  for(size_t i = 0; i < commands_.size(); ++i)
  {
    const auto & c = commands_[i];
    file << "    {\n";
    file << "      \"index\": " << i << ",\n";
    file << "      \"kp\": " << c.kp << ",\n";
    file << "      \"kd\": " << c.kd << ",\n";

    file << "      \"position\": [";
    for(size_t j = 0; j < c.position.size(); ++j)
    {
      file << c.position[j];
      if(j + 1 < c.position.size())
      {
        file << ", ";
      }
    }
    file << "],\n";

    file << "      \"velocity\": [";
    for(size_t j = 0; j < c.velocity.size(); ++j)
    {
      file << c.velocity[j];
      if(j + 1 < c.velocity.size())
      {
        file << ", ";
      }
    }
    file << "],\n";

    file << "      \"torque\": [";
    for(size_t j = 0; j < c.torque.size(); ++j)
    {
      file << c.torque[j];
      if(j + 1 < c.torque.size())
      {
        file << ", ";
      }
    }
    file << "]\n";

    file << "    }";
    if(i + 1 < commands_.size())
    {
      file << ",";
    }
    file << "\n";
  }
  file << "  ],\n";

  // ── Summary ──
  file << "  \"summary\": {\n";
  file << "    \"total_states_sent\": " << states_.size() << ",\n";
  file << "    \"total_commands_received\": " << commands_.size() << "\n";
  file << "  }\n";

  file << "}\n";
  file.close();

  mc_rtc::log::success("Log saved to {} ({} states, {} commands)", filename, states_.size(), commands_.size());
}

} // namespace mc_communication

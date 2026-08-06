#include <robot_comm/serialization/FlatbufferSerializer.h>

#include <iostream>
#include <type_traits>
#include <variant>

namespace robot_comm
{

ByteBuffer FlatbufferSerializer::serializeImpl(MessageType type, const void * msg)
{
  switch(type)
  {
    case MessageType::CONFIG:
    {
      auto str = static_cast<const std::string *>(msg);

      flatbuffers::FlatBufferBuilder builder;

      auto config = builder.CreateString(*str);

      auto message = robot_comm_msgs::CreateMessageConfig(builder, config);

      builder.Finish(message);

      return ByteBuffer(builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize());
    }
    case MessageType::STATE:
    {
      auto message_state = static_cast<const State *>(msg);

      flatbuffers::FlatBufferBuilder builder;

      auto position = builder.CreateVector(message_state->position);
      auto velocity = builder.CreateVector(message_state->velocity);
      auto torque = builder.CreateVector(message_state->torque);

      auto message = robot_comm_msgs::CreateMessageState(builder, position, velocity, torque);
      builder.Finish(message);

      return ByteBuffer(builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize());
    }
    case MessageType::COMMAND:
    {
      auto message_cmd = static_cast<const Command *>(msg);

      flatbuffers::FlatBufferBuilder builder;

      auto position = builder.CreateVector(message_cmd->position);
      auto velocity = builder.CreateVector(message_cmd->velocity);
      auto torque = builder.CreateVector(message_cmd->torque);

      auto message =
          robot_comm_msgs::CreateMessageCommand(builder, message_cmd->kp, message_cmd->kd, position, velocity, torque);
      builder.Finish(message);

      return ByteBuffer(builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize());
    }
    case MessageType::ENVELOPE:
    {
      auto envelope = static_cast<const Envelope *>(msg);

      flatbuffers::FlatBufferBuilder builder;

      auto topic = builder.CreateString(envelope->topic);

      robot_comm_msgs::Payload payload_type = robot_comm_msgs::Payload_NONE;

      flatbuffers::Offset<void> payload;

      std::visit(
          [&](const auto & data)
          {
            using T = std::decay_t<decltype(data)>;

            if constexpr(std::is_same_v<T, std::string>)
            {
              auto config = builder.CreateString(data);

              auto msg = robot_comm_msgs::CreateMessageConfig(builder, config);

              payload_type = robot_comm_msgs::Payload_MessageConfig;

              payload = msg.Union();
            }
            else if constexpr(std::is_same_v<T, State>)
            {
              auto position = builder.CreateVector(data.position);

              auto velocity = builder.CreateVector(data.velocity);

              auto torque = builder.CreateVector(data.torque);

              auto msg = robot_comm_msgs::CreateMessageState(builder, position, velocity, torque);

              payload_type = robot_comm_msgs::Payload_MessageState;

              payload = msg.Union();
            }
            else if constexpr(std::is_same_v<T, Command>)
            {
              auto position = builder.CreateVector(data.position);

              auto velocity = builder.CreateVector(data.velocity);

              auto torque = builder.CreateVector(data.torque);

              auto msg = robot_comm_msgs::CreateMessageCommand(builder, data.kp, data.kd, position, velocity, torque);

              payload_type = robot_comm_msgs::Payload_MessageCommand;

              payload = msg.Union();
            }
          },
          envelope->payload);

      auto message = robot_comm_msgs::CreateMessageEnvelope(builder, topic, envelope->timestamp, payload_type, payload);

      builder.Finish(message);

      return ByteBuffer(builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize());
    }

    default:
      return {};
  }
}

std::optional<void *> FlatbufferSerializer::deserializeImpl(MessageType type, const uint8_t * data, size_t size)
{
  switch(type)
  {
    case MessageType::STATE:
    {
      auto fb = flatbuffers::GetRoot<robot_comm_msgs::MessageState>(data);

      auto * state = new State();

      state->position.assign(fb->position()->begin(), fb->position()->end());

      state->velocity.assign(fb->velocity()->begin(), fb->velocity()->end());

      state->torque.assign(fb->torque()->begin(), fb->torque()->end());

      return static_cast<void *>(state);
    }

    case MessageType::COMMAND:
    {
      auto fb = flatbuffers::GetRoot<robot_comm_msgs::MessageCommand>(data);

      auto * cmd = new Command();

      cmd->kp = fb->kp();
      cmd->kd = fb->kd();

      cmd->position.assign(fb->position()->begin(), fb->position()->end());

      cmd->velocity.assign(fb->velocity()->begin(), fb->velocity()->end());

      cmd->torque.assign(fb->torque()->begin(), fb->torque()->end());

      return static_cast<void *>(cmd);
    }

    case MessageType::CONFIG:
    {
      auto fb = flatbuffers::GetRoot<robot_comm_msgs::MessageConfig>(data);

      auto * config = new std::string(fb->config()->str());

      return static_cast<void *>(config);
    }

    case MessageType::ENVELOPE:
    {
      auto fb = flatbuffers::GetRoot<robot_comm_msgs::MessageEnvelope>(data);

      auto * envelope = new Envelope();

      envelope->topic = fb->topic()->str();

      envelope->timestamp = fb->timestamp();

      switch(fb->payload_type())
      {
        case robot_comm_msgs::Payload_MessageConfig:
        {
          auto config = fb->payload_as_MessageConfig();

          envelope->payload = config->config()->str();

          break;
        }

        case robot_comm_msgs::Payload_MessageState:
        {
          auto state_fb = fb->payload_as_MessageState();

          State state;

          state.position.assign(state_fb->position()->begin(), state_fb->position()->end());

          state.velocity.assign(state_fb->velocity()->begin(), state_fb->velocity()->end());

          state.torque.assign(state_fb->torque()->begin(), state_fb->torque()->end());

          envelope->payload = std::move(state);

          break;
        }

        case robot_comm_msgs::Payload_MessageCommand:
        {
          auto cmd_fb = fb->payload_as_MessageCommand();

          Command cmd;

          cmd.kp = cmd_fb->kp();
          cmd.kd = cmd_fb->kd();

          cmd.position.assign(cmd_fb->position()->begin(), cmd_fb->position()->end());

          cmd.velocity.assign(cmd_fb->velocity()->begin(), cmd_fb->velocity()->end());

          cmd.torque.assign(cmd_fb->torque()->begin(), cmd_fb->torque()->end());

          envelope->payload = std::move(cmd);

          break;
        }

        default:
        {
          envelope->payload = std::monostate{};

          break;
        }
      }

      return static_cast<void *>(envelope);
    }

    default:
      return std::nullopt;
  }
}

} // namespace robot_comm

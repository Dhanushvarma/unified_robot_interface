#include <mc_communication/serialization/ProtobufSerializer.h>

#include <mc_communication/serialization/protobuf/mc_rtc_msgs.pb.h>

#include <variant>

namespace mc_communication
{

ByteBuffer ProtobufSerializer::serializeImpl(MessageType type, const void * msg)
{
  switch(type)
  {
    case MessageType::CONFIG:
    {
      auto str = static_cast<const std::string *>(msg);

      mc_communication_msgs_proto::MessageConfig pb;
      pb.set_config(*str);

      return serializeMessage(pb);
    }

    case MessageType::STATE:
    {
      auto state = static_cast<const State *>(msg);

      mc_communication_msgs_proto::MessageState pb;

      pb.mutable_position()->Assign(state->position.begin(), state->position.end());
      pb.mutable_velocity()->Assign(state->velocity.begin(), state->velocity.end());
      pb.mutable_torque()->Assign(state->torque.begin(), state->torque.end());

      return serializeMessage(pb);
    }

    case MessageType::COMMAND:
    {
      auto cmd = static_cast<const Command *>(msg);

      mc_communication_msgs_proto::MessageCommand pb;

      pb.set_kp(cmd->kp);
      pb.set_kd(cmd->kd);

      pb.mutable_position()->Assign(cmd->position.begin(), cmd->position.end());
      pb.mutable_velocity()->Assign(cmd->velocity.begin(), cmd->velocity.end());
      pb.mutable_torque()->Assign(cmd->torque.begin(), cmd->torque.end());

      return serializeMessage(pb);
    }

    case MessageType::ENVELOPE:
    {
      auto envelope = static_cast<const Envelope *>(msg);

      mc_communication_msgs_proto::MessageEnvelope pb;

      pb.set_topic(envelope->topic);
      pb.set_timestamp(envelope->timestamp);

      std::visit(
          [&](const auto & data)
          {
            using T = std::decay_t<decltype(data)>;

            if constexpr(std::is_same_v<T, std::string>)
            {
              pb.mutable_config()->set_config(data);
            }
            else if constexpr(std::is_same_v<T, State>)
            {
              auto * state_pb = pb.mutable_state();

              state_pb->mutable_position()->Assign(data.position.begin(), data.position.end());
              state_pb->mutable_velocity()->Assign(data.velocity.begin(), data.velocity.end());
              state_pb->mutable_torque()->Assign(data.torque.begin(), data.torque.end());
            }
            else if constexpr(std::is_same_v<T, Command>)
            {
              auto * cmd_pb = pb.mutable_command();

              cmd_pb->set_kp(data.kp);
              cmd_pb->set_kd(data.kd);

              cmd_pb->mutable_position()->Assign(data.position.begin(), data.position.end());
              cmd_pb->mutable_velocity()->Assign(data.velocity.begin(), data.velocity.end());
              cmd_pb->mutable_torque()->Assign(data.torque.begin(), data.torque.end());
            }
          },
          envelope->payload);

      return serializeMessage(pb);
    }

    default:
      return {};
  }
}

std::optional<void *> ProtobufSerializer::deserializeImpl(MessageType type, const uint8_t * data, size_t size)
{
  switch(type)
  {
    case MessageType::CONFIG:
    {
      auto pb = deserializeMessage<mc_communication_msgs_proto::MessageConfig>(data, size);
      if(!pb) return std::nullopt;

      return static_cast<void *>(new std::string(pb->config()));
    }

    case MessageType::STATE:
    {
      auto pb = deserializeMessage<mc_communication_msgs_proto::MessageState>(data, size);
      if(!pb) return std::nullopt;

      auto * state = new State();

      state->position.assign(pb->position().begin(), pb->position().end());
      state->velocity.assign(pb->velocity().begin(), pb->velocity().end());
      state->torque.assign(pb->torque().begin(), pb->torque().end());

      return static_cast<void *>(state);
    }

    case MessageType::COMMAND:
    {
      auto pb = deserializeMessage<mc_communication_msgs_proto::MessageCommand>(data, size);
      if(!pb) return std::nullopt;

      auto * cmd = new Command();

      cmd->kp = pb->kp();
      cmd->kd = pb->kd();

      cmd->position.assign(pb->position().begin(), pb->position().end());
      cmd->velocity.assign(pb->velocity().begin(), pb->velocity().end());
      cmd->torque.assign(pb->torque().begin(), pb->torque().end());

      return static_cast<void *>(cmd);
    }

    case MessageType::ENVELOPE:
    {
      auto pb = deserializeMessage<mc_communication_msgs_proto::MessageEnvelope>(data, size);
      if(!pb) return std::nullopt;

      auto * envelope = new Envelope();

      envelope->topic = pb->topic();
      envelope->timestamp = pb->timestamp();

      switch(pb->payload_case())
      {
        case mc_communication_msgs_proto::MessageEnvelope::kConfig:
        {
          envelope->payload = pb->config().config();
          break;
        }

        case mc_communication_msgs_proto::MessageEnvelope::kState:
        {
          State state;

          state.position.assign(pb->state().position().begin(), pb->state().position().end());

          state.velocity.assign(pb->state().velocity().begin(), pb->state().velocity().end());

          state.torque.assign(pb->state().torque().begin(), pb->state().torque().end());

          envelope->payload = std::move(state);
          break;
        }

        case mc_communication_msgs_proto::MessageEnvelope::kCommand:
        {
          Command cmd;

          cmd.kp = pb->command().kp();
          cmd.kd = pb->command().kd();

          cmd.position.assign(pb->command().position().begin(), pb->command().position().end());

          cmd.velocity.assign(pb->command().velocity().begin(), pb->command().velocity().end());

          cmd.torque.assign(pb->command().torque().begin(), pb->command().torque().end());

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

} // namespace mc_communication

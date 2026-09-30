#include <robot_comm/serialization/ProtobufSerializer.h>

#include <robot_comm/serialization/protobuf/mc_rtc_msgs.pb.h>

#include <algorithm>
#include <variant>

namespace robot_comm
{

namespace
{
template<size_t N>
void copyInto(const google::protobuf::RepeatedField<double> & src, std::array<double, N> & dst)
{
  std::copy_n(src.begin(), std::min<size_t>(src.size(), N), dst.begin());
}

void writeState(const State & state, robot_comm_msgs_proto::MessageState & pb)
{
  pb.set_stamp(state.stamp);
  pb.mutable_position()->Assign(state.position.begin(), state.position.end());
  pb.mutable_velocity()->Assign(state.velocity.begin(), state.velocity.end());
  pb.mutable_torque()->Assign(state.torque.begin(), state.torque.end());

  for(const auto & s : state.bodySensors)
  {
    auto * bs = pb.add_body_sensors();
    bs->set_name(s.name);
    bs->mutable_orientation()->Assign(s.orientation.begin(), s.orientation.end());
    bs->mutable_angular_velocity()->Assign(s.angularVelocity.begin(), s.angularVelocity.end());
    bs->mutable_linear_acceleration()->Assign(s.linearAcceleration.begin(), s.linearAcceleration.end());
  }

  for(const auto & s : state.forceSensors)
  {
    auto * fs = pb.add_force_sensors();
    fs->set_name(s.name);
    fs->mutable_force()->Assign(s.force.begin(), s.force.end());
    fs->mutable_torque()->Assign(s.torque.begin(), s.torque.end());
  }
}

void readState(const robot_comm_msgs_proto::MessageState & pb, State & state)
{
  state.position.assign(pb.position().begin(), pb.position().end());
  state.velocity.assign(pb.velocity().begin(), pb.velocity().end());
  state.torque.assign(pb.torque().begin(), pb.torque().end());
  state.stamp = pb.stamp();

  state.bodySensors.reserve(pb.body_sensors_size());
  for(const auto & s : pb.body_sensors())
  {
    BodySensorData bs;
    bs.name = s.name();
    copyInto(s.orientation(), bs.orientation);
    copyInto(s.angular_velocity(), bs.angularVelocity);
    copyInto(s.linear_acceleration(), bs.linearAcceleration);
    state.bodySensors.push_back(std::move(bs));
  }

  state.forceSensors.reserve(pb.force_sensors_size());
  for(const auto & s : pb.force_sensors())
  {
    ForceSensorData fs;
    fs.name = s.name();
    copyInto(s.force(), fs.force);
    copyInto(s.torque(), fs.torque);
    state.forceSensors.push_back(std::move(fs));
  }
}
} // namespace

ByteBuffer ProtobufSerializer::serializeImpl(MessageType type, const void * msg)
{
  switch(type)
  {
    case MessageType::CONFIG:
    {
      auto str = static_cast<const std::string *>(msg);

      robot_comm_msgs_proto::MessageConfig pb;
      pb.set_config(*str);

      return serializeMessage(pb);
    }

    case MessageType::STATE:
    {
      auto state = static_cast<const State *>(msg);

      robot_comm_msgs_proto::MessageState pb;
      writeState(*state, pb);

      return serializeMessage(pb);
    }

    case MessageType::COMMAND:
    {
      auto cmd = static_cast<const Command *>(msg);

      robot_comm_msgs_proto::MessageCommand pb;

      pb.set_kp(cmd->kp);
      pb.set_kd(cmd->kd);

      pb.mutable_position()->Assign(cmd->position.begin(), cmd->position.end());
      pb.mutable_velocity()->Assign(cmd->velocity.begin(), cmd->velocity.end());
      pb.mutable_torque()->Assign(cmd->torque.begin(), cmd->torque.end());
      pb.set_state_stamp(cmd->stateStamp);
      pb.set_state_hold(cmd->stateHold);

      return serializeMessage(pb);
    }

    case MessageType::ENVELOPE:
    {
      auto envelope = static_cast<const Envelope *>(msg);

      robot_comm_msgs_proto::MessageEnvelope pb;

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
              writeState(data, *pb.mutable_state());
            }
            else if constexpr(std::is_same_v<T, Command>)
            {
              auto * cmd_pb = pb.mutable_command();

              cmd_pb->set_kp(data.kp);
              cmd_pb->set_kd(data.kd);

              cmd_pb->mutable_position()->Assign(data.position.begin(), data.position.end());
              cmd_pb->mutable_velocity()->Assign(data.velocity.begin(), data.velocity.end());
              cmd_pb->mutable_torque()->Assign(data.torque.begin(), data.torque.end());
              cmd_pb->set_state_stamp(data.stateStamp);
              cmd_pb->set_state_hold(data.stateHold);
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
      auto pb = deserializeMessage<robot_comm_msgs_proto::MessageConfig>(data, size);
      if(!pb) return std::nullopt;

      return static_cast<void *>(new std::string(pb->config()));
    }

    case MessageType::STATE:
    {
      auto pb = deserializeMessage<robot_comm_msgs_proto::MessageState>(data, size);
      if(!pb) return std::nullopt;

      auto * state = new State();
      readState(*pb, *state);

      return static_cast<void *>(state);
    }

    case MessageType::COMMAND:
    {
      auto pb = deserializeMessage<robot_comm_msgs_proto::MessageCommand>(data, size);
      if(!pb) return std::nullopt;

      auto * cmd = new Command();

      cmd->kp = pb->kp();
      cmd->kd = pb->kd();

      cmd->position.assign(pb->position().begin(), pb->position().end());
      cmd->velocity.assign(pb->velocity().begin(), pb->velocity().end());
      cmd->torque.assign(pb->torque().begin(), pb->torque().end());
      cmd->stateStamp = pb->state_stamp();
      cmd->stateHold = pb->state_hold();

      return static_cast<void *>(cmd);
    }

    case MessageType::ENVELOPE:
    {
      auto pb = deserializeMessage<robot_comm_msgs_proto::MessageEnvelope>(data, size);
      if(!pb) return std::nullopt;

      auto * envelope = new Envelope();

      envelope->topic = pb->topic();
      envelope->timestamp = pb->timestamp();

      switch(pb->payload_case())
      {
        case robot_comm_msgs_proto::MessageEnvelope::kConfig:
        {
          envelope->payload = pb->config().config();
          break;
        }

        case robot_comm_msgs_proto::MessageEnvelope::kState:
        {
          State state;
          readState(pb->state(), state);
          envelope->payload = std::move(state);
          break;
        }

        case robot_comm_msgs_proto::MessageEnvelope::kCommand:
        {
          Command cmd;

          cmd.kp = pb->command().kp();
          cmd.kd = pb->command().kd();

          cmd.position.assign(pb->command().position().begin(), pb->command().position().end());

          cmd.velocity.assign(pb->command().velocity().begin(), pb->command().velocity().end());

          cmd.torque.assign(pb->command().torque().begin(), pb->command().torque().end());
          cmd.stateStamp = pb->command().state_stamp();
          cmd.stateHold = pb->command().state_hold();

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

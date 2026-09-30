#include <robot_comm/serialization/FlatbufferSerializer.h>

#include <algorithm>
#include <iostream>
#include <type_traits>
#include <variant>

namespace robot_comm
{

namespace
{
template<size_t N>
void copyInto(const flatbuffers::Vector<double> * src, std::array<double, N> & dst)
{
  if(!src) return;
  std::copy_n(src->begin(), std::min<size_t>(src->size(), N), dst.begin());
}

flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<robot_comm_msgs::MessageBodySensor>>> createBodySensors(
    flatbuffers::FlatBufferBuilder & builder,
    const std::vector<BodySensorData> & sensors)
{
  std::vector<flatbuffers::Offset<robot_comm_msgs::MessageBodySensor>> offsets;
  offsets.reserve(sensors.size());
  for(const auto & s : sensors)
  {
    auto name = builder.CreateString(s.name);
    auto orientation = builder.CreateVector(s.orientation.data(), s.orientation.size());
    auto angularVelocity = builder.CreateVector(s.angularVelocity.data(), s.angularVelocity.size());
    auto linearAcceleration = builder.CreateVector(s.linearAcceleration.data(), s.linearAcceleration.size());
    offsets.push_back(
        robot_comm_msgs::CreateMessageBodySensor(builder, name, orientation, angularVelocity, linearAcceleration));
  }
  return builder.CreateVector(offsets);
}

std::vector<BodySensorData> parseBodySensors(
    const flatbuffers::Vector<flatbuffers::Offset<robot_comm_msgs::MessageBodySensor>> * fb_sensors)
{
  std::vector<BodySensorData> sensors;
  if(!fb_sensors) return sensors;
  sensors.reserve(fb_sensors->size());
  for(const auto * s : *fb_sensors)
  {
    BodySensorData bs;
    if(s->name()) bs.name = s->name()->str();
    copyInto(s->orientation(), bs.orientation);
    copyInto(s->angular_velocity(), bs.angularVelocity);
    copyInto(s->linear_acceleration(), bs.linearAcceleration);
    sensors.push_back(std::move(bs));
  }
  return sensors;
}

flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<robot_comm_msgs::MessageForceSensor>>> createForceSensors(
    flatbuffers::FlatBufferBuilder & builder,
    const std::vector<ForceSensorData> & sensors)
{
  std::vector<flatbuffers::Offset<robot_comm_msgs::MessageForceSensor>> offsets;
  offsets.reserve(sensors.size());
  for(const auto & s : sensors)
  {
    auto name = builder.CreateString(s.name);
    auto force = builder.CreateVector(s.force.data(), s.force.size());
    auto torque = builder.CreateVector(s.torque.data(), s.torque.size());
    offsets.push_back(robot_comm_msgs::CreateMessageForceSensor(builder, name, force, torque));
  }
  return builder.CreateVector(offsets);
}

std::vector<ForceSensorData> parseForceSensors(
    const flatbuffers::Vector<flatbuffers::Offset<robot_comm_msgs::MessageForceSensor>> * fb_sensors)
{
  std::vector<ForceSensorData> sensors;
  if(!fb_sensors) return sensors;
  sensors.reserve(fb_sensors->size());
  for(const auto * s : *fb_sensors)
  {
    ForceSensorData fs;
    if(s->name()) fs.name = s->name()->str();
    copyInto(s->force(), fs.force);
    copyInto(s->torque(), fs.torque);
    sensors.push_back(std::move(fs));
  }
  return sensors;
}
} // namespace

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
      auto bodySensors = createBodySensors(builder, message_state->bodySensors);
      auto forceSensors = createForceSensors(builder, message_state->forceSensors);

      auto message = robot_comm_msgs::CreateMessageState(builder, position, velocity, torque, bodySensors, forceSensors,
                                                         message_state->stamp);
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
          robot_comm_msgs::CreateMessageCommand(builder, message_cmd->kp, message_cmd->kd, position, velocity, torque,
                                                message_cmd->stateStamp, message_cmd->stateHold);
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
              auto bodySensors = createBodySensors(builder, data.bodySensors);
              auto forceSensors = createForceSensors(builder, data.forceSensors);
              auto msg = robot_comm_msgs::CreateMessageState(builder, position, velocity, torque, bodySensors,
                                                             forceSensors, data.stamp);

              payload_type = robot_comm_msgs::Payload_MessageState;
              payload = msg.Union();
            }
            else if constexpr(std::is_same_v<T, Command>)
            {
              auto position = builder.CreateVector(data.position);
              auto velocity = builder.CreateVector(data.velocity);
              auto torque = builder.CreateVector(data.torque);
              auto msg = robot_comm_msgs::CreateMessageCommand(builder, data.kp, data.kd, position, velocity, torque,
                                                               data.stateStamp, data.stateHold);

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
      state->bodySensors = parseBodySensors(fb->body_sensors());
      state->forceSensors = parseForceSensors(fb->force_sensors());
      state->stamp = fb->stamp();

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
      cmd->stateStamp = fb->state_stamp();
      cmd->stateHold = fb->state_hold();

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
          state.bodySensors = parseBodySensors(state_fb->body_sensors());
          state.forceSensors = parseForceSensors(state_fb->force_sensors());
          state.stamp = state_fb->stamp();
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
          cmd.stateStamp = cmd_fb->state_stamp();
          cmd.stateHold = cmd_fb->state_hold();
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

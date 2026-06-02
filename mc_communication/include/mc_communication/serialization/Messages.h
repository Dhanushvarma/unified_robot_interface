#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace mc_communication
{
static constexpr uint32_t MESSAGE_MAGIC = 0x4D434F4D; // "MCOM"

enum class SerializationBackend : uint8_t
{
  Flatbuffer,
  Protobuf
};

enum class Protocol : uint8_t
{
  TCP,
  UDP,
  ZENOH,
  ZENOH_SHM,
  SHM
};

enum class MessageType : uint16_t
{
  CONFIG,
  STATE,
  COMMAND,
  ENVELOPE
};

struct State
{
  std::vector<double> position;
  std::vector<double> velocity;
  std::vector<double> torque;
};

struct Command
{
  double kp;
  double kd;
  std::vector<double> position;
  std::vector<double> velocity;
  std::vector<double> torque;
};

struct Envelope
{
  std::string topic;
  uint64_t timestamp;

  std::variant<std::monostate, State, Command, std::string> payload;
};

struct MessageHeader
{
  uint32_t magic = MESSAGE_MAGIC;
  uint16_t version = 1;
  MessageType type;
  SerializationBackend backend;
  uint32_t size = 0;
};

} // namespace mc_communication

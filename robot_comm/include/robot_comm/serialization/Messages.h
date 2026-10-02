#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace robot_comm
{
static constexpr uint32_t MESSAGE_MAGIC = 0x4D434F4D; // "MCOM"

/// Serialization format, set by `network_interface.backend`.
enum class SerializationBackend : uint8_t
{
  Flatbuffer,
  Protobuf
};

/// Transport, set by `network_interface.protocol`. Only ZENOH and ZENOH_SHM
/// are implemented.
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

/// One IMU reading (see robot_interface::IMUData).
struct BodySensorData
{
  /// RobotModule body sensor name, e.g. "Accelerometer".
  std::string name;
  /// Quaternion (w, x, y, z).
  std::array<double, 4> orientation = {1.0, 0.0, 0.0, 0.0};
  /// [rad/s]
  std::array<double, 3> angularVelocity = {0.0, 0.0, 0.0};
  /// [m/s²]
  std::array<double, 3> linearAcceleration = {0.0, 0.0, 0.0};
};

/// One force/torque reading in the sensor frame (see robot_interface::WrenchData).
struct ForceSensorData
{
  /// RobotModule force sensor name, e.g. "EEForceSensor".
  std::string name;
  /// [N]
  std::array<double, 3> force = {0.0, 0.0, 0.0};
  /// [Nm]
  std::array<double, 3> torque = {0.0, 0.0, 0.0};
};

/// Robot state, sent by `uri interface` on {name}/state every robot cycle.
/// Joint vectors follow the RobotModule's reference joint order.
struct State
{
  /// Joint positions [rad].
  std::vector<double> position;
  /// Joint velocities [rad/s]. May be empty.
  std::vector<double> velocity;
  /// Joint torques [Nm].
  std::vector<double> torque;
  /// IMU readings.
  std::vector<BodySensorData> bodySensors;
  /// Force/torque sensor readings.
  std::vector<ForceSensorData> forceSensors;
  /// Sender's steady clock [ns] when this state was sent, echoed back in
  /// Command::stateStamp to measure the round trip. 0 = not set.
  uint64_t stamp = 0;
};

/// Robot command, sent by `uri manager` on {name}/command. Only the vector
/// matching the robot's `controller.mode` is filled.
struct Command
{
  /// Reserved.
  double kp;
  /// Reserved.
  double kd;
  /// Joint positions [rad].
  std::vector<double> position;
  /// Joint velocities [rad/s].
  std::vector<double> velocity;
  /// Joint torques [Nm].
  std::vector<double> torque;
  /// State::stamp of the latest state the manager had received (0 = none yet).
  uint64_t stateStamp = 0;
  /// How long [ns] the manager held that state before sending this command.
  /// The interface measures the round trip as now - stateStamp - stateHold.
  uint64_t stateHold = 0;
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

} // namespace robot_comm

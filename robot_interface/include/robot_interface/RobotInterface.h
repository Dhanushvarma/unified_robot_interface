#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <mc_rtc/Configuration.h>

#include <robot_comm/CommunicationFactory.h>
#include <robot_comm/serialization/MessageTraits.h>
#include <robot_interface/PluginLoader.h>
#include <robot_interface/RobotDriverTemplate.h>

namespace mc_robot_interface
{

// Always-running robot-side process.
//
// Lifecycle:
//   1. Starts and connects via Zenoh (client role).
//   2. Registers a queryable on "{name}/init".
//   3. Blocks until RobotManager sends an init query (robot config as YAML).
//   4. Loads the requested RobotDriver plugin.
//   5. Replies "OK" or "ERROR: <reason>" to the init query.
//   6. Runs the real-time control loop:
//        updateSensors() -> publish state on "{name}/state"
//        updateControl() -> apply latest command from "{name}/command" to driver
//
// Hot-plug tools (future):
//   Register a queryable on "{name}/tool". The same PluginLoader<T> pattern
//   is used to load tool plugins at runtime.
class RobotInterface
{
public:
  RobotInterface(const std::string & name, const mc_rtc::Configuration & comm_config);

  // Block until interrupt is set. Handles init handshake then runs control loop.
  void run(const std::atomic<bool> & interrupt);

  const std::string & name() const
  {
    return name_;
  }

private:
  // Synchronous query handler registered on "{name}/init".
  // Payload: serialized config string (YAML). Returns "OK" or "ERROR: <reason>".
  robot_comm::ByteBuffer handleInitQuery(const robot_comm::ByteBuffer & payload);

  void loadDriver(const std::string & driver_name, const mc_rtc::Configuration & driver_config);

  void updateSensors();
  void updateControl();

  std::string name_;
  mc_rtc::Configuration comm_config_;
  std::unique_ptr<robot_comm::Communication> comm_;

  PluginLoader<RobotDriver> driver_loader_;
  std::shared_ptr<RobotDriver> driver_;

  // Last received command — re-applied every RTDE cycle to give the robot a
  // continuous stream even when the manager sends at a slower rate than RTDE.
  std::optional<robot_comm::Command> last_cmd_;

  // Signalled when the init query has been handled and driver is ready.
  std::mutex init_mutex_;
  std::condition_variable init_cv_;
  bool initialized_ = false;
  std::string init_error_;
};

} // namespace mc_robot_interface

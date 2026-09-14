#include <robot_interface/RobotInterface.h>

#include <robot_comm/CommunicationZenoh.h>
#include <robot_comm/serialization/MessageTraits.h>

#include <mc_rtc/logging.h>
#include <robot_interface/config.h>

#include <chrono>

namespace mc_robot_interface
{

RobotInterface::RobotInterface(const std::string & name, const mc_rtc::Configuration & comm_config)
: name_(name), comm_config_(comm_config),
  driver_loader_("MC_RTC_ROBOT_DRIVER", {mc_robot_interface::MC_ROBOT_INTERFACE_INSTALL_PREFIX}, true)
{
  comm_ = robot_comm::CommunicationFactory::makeCommunication(name_, comm_config_);
  comm_->setupClient();
}

void RobotInterface::run(const std::atomic<bool> & interrupt)
{
  // Register queryable so RobotManager can bootstrap us.
  const std::string init_topic = name_ + "/init";
  comm_->handleQuery(init_topic, [this](const robot_comm::ByteBuffer & payload) -> robot_comm::ByteBuffer
                     { return handleInitQuery(payload); });

  mc_rtc::log::info("[RobotInterface] '{}' waiting for init query on '{}'", name_, init_topic);

  // Block until the init query is handled (or we are interrupted).
  // Use wait_for so the interrupt flag is checked even if init_cv_ is never
  // notified (e.g. when no manager is running and Ctrl+C is pressed).
  {
    std::unique_lock<std::mutex> lock(init_mutex_);
    while(!initialized_ && init_error_.empty() && !interrupt.load())
      init_cv_.wait_for(lock, std::chrono::milliseconds(100));
  }

  if(interrupt.load())
  {
    mc_rtc::log::warning("[RobotInterface] '{}' interrupted before init", name_);
    return;
  }

  if(!init_error_.empty())
  {
    mc_rtc::log::error("[RobotInterface] '{}' init failed: {}", name_, init_error_);
    return;
  }

  mc_rtc::log::success("[RobotInterface] '{}' initialized, starting control loop", name_);

  // Prime the last command with the robot's current position so the driver's
  // reverse interface gets servoJ right away and the URScript doesn't time out
  // while waiting for the first command from the manager.
  driver_->sync();
  auto init_q = driver_->getActualQ();
  if(!init_q.empty())
  {
    robot_comm::Command hold;
    hold.position = init_q;
    last_cmd_ = std::move(hold);
  }

  // Control loop: run at driver's natural rate (spin freely, driver sync() paces us).
  while(!interrupt.load())
  {
    driver_->sync();
    // Shutdown may have been requested while sync() was blocking.
    if(interrupt.load())
    {
      break;
    }

    updateSensors();
    updateControl();
  }

  mc_rtc::log::info("[RobotInterface] '{}' control loop stopped", name_);
}

robot_comm::ByteBuffer RobotInterface::handleInitQuery(const robot_comm::ByteBuffer & payload)
{
  // Deserialize config string from payload.
  auto config_opt =
      comm_->serializer()->deserialize<std::string>(robot_comm::MessageType::CONFIG, payload.data(), payload.size());

  if(!config_opt)
  {
    std::string err = "Failed to deserialize init config";
    mc_rtc::log::error("[RobotInterface] '{}' init query: {}", name_, err);
    std::lock_guard<std::mutex> lock(init_mutex_);
    init_error_ = err;
    init_cv_.notify_one();
    return robot_comm::ByteBuffer(err.begin(), err.end());
  }

  mc_rtc::Configuration robot_config;
  try
  {
    robot_config.loadData(*config_opt);
  }
  catch(const std::exception & e)
  {
    std::string err = std::string("Invalid config YAML: ") + e.what();
    mc_rtc::log::error("[RobotInterface] '{}' init query: {}", name_, err);
    std::lock_guard<std::mutex> lock(init_mutex_);
    init_error_ = err;
    init_cv_.notify_one();
    return robot_comm::ByteBuffer(err.begin(), err.end());
  }

  const std::string driver_name = robot_config("robot_interface")("driver", std::string{});
  if(driver_name.empty())
  {
    std::string err = "Missing 'robot_interface.driver' in config";
    mc_rtc::log::error("[RobotInterface] '{}' init query: {}", name_, err);
    std::lock_guard<std::mutex> lock(init_mutex_);
    init_error_ = err;
    init_cv_.notify_one();
    return robot_comm::ByteBuffer(err.begin(), err.end());
  }

  try
  {
    loadDriver(driver_name, robot_config("robot_interface"));
  }
  catch(const std::exception & e)
  {
    std::string err = std::string("Driver load failed: ") + e.what();
    mc_rtc::log::error("[RobotInterface] '{}' {}", name_, err);
    std::lock_guard<std::mutex> lock(init_mutex_);
    init_error_ = err;
    init_cv_.notify_one();
    return robot_comm::ByteBuffer(err.begin(), err.end());
  }

  {
    std::lock_guard<std::mutex> lock(init_mutex_);
    initialized_ = true;
  }
  init_cv_.notify_one();

  static const std::string ok = "OK";
  return robot_comm::ByteBuffer(ok.begin(), ok.end());
}

void RobotInterface::loadDriver(const std::string & driver_name, const mc_rtc::Configuration & driver_config)
{
  const std::string ip = driver_config("ip", std::string{"127.0.0.1"});
  const uint16_t port = driver_config("port", uint16_t{0});

  mc_rtc::log::info("[RobotInterface] '{}' loading driver '{}' ({}:{})", name_, driver_name, ip, port);

  driver_ = driver_loader_.load(driver_name, ip, port);

  mc_rtc::log::success("[RobotInterface] '{}' driver '{}' loaded", name_, driver_name);
}

void RobotInterface::updateSensors()
{
  if(!driver_) return;

  robot_comm::State state;
  state.position = driver_->getActualQ();
  state.velocity = driver_->getActualQd();
  state.torque = driver_->getJointTorques();

  if(state.position.empty()) return;

  comm_->send(comm_->encode(state));
}

void RobotInterface::updateControl()
{
  auto rx = comm_->receive();
  if(rx && !rx->empty())
  {
    auto cmd_opt =
        comm_->serializer()->deserialize<robot_comm::Command>(robot_comm::MessageType::COMMAND, rx->data(), rx->size());
    if(cmd_opt)
    {
      last_cmd_ = std::move(cmd_opt);
    }
  }

  if(!last_cmd_) return;

  if(!last_cmd_->position.empty())
    driver_->servoJ(last_cmd_->position);
  else if(!last_cmd_->velocity.empty())
    driver_->speedJ(last_cmd_->velocity);
}

} // namespace mc_robot_interface

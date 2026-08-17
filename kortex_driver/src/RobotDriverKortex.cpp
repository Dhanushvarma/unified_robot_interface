#include <kortex_driver/RobotDriverKortex.h>

#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <iostream>
#include <stdexcept>

namespace
{

constexpr double pi = 3.14159265358979323846;

double degToRad(double value)
{
  return value * pi / 180.0;
}

double radToDeg(double value)
{
  return value * 180.0 / pi;
}

} // namespace

namespace kortex_driver
{

// TODO: implement username and password
RobotDriverKortex::RobotDriverKortex(const std::string & ip, uint16_t port)
: ip_(ip), tcp_port_(port == 0 ? default_tcp_port_ : port)
{
  fmt::print("[RobotDriverKortex] Connecting to Kinova robot at {}:{}\n", ip_, tcp_port_);

  try
  {
    connect();
  }
  catch(...)
  {
    disconnect();
    throw;
  }

  fmt::print("[RobotDriverKortex] Connected to {}\n", ip_);
}

RobotDriverKortex::~RobotDriverKortex()
{
  fmt::print("[RobotDriverKortex] Disconnecting from Kinova robot at {}\n", ip_);

  disconnect();

  fmt::print("[RobotDriverKortex] Disconnected from {}\n", ip_);
}

void RobotDriverKortex::sync()
{
  if(!command_initialized_)
  {
    initializeCyclicCommand();
    return;
  }

  auto frame_id = static_cast<uint16_t>(command_.frame_id() + 1);

  if(frame_id == 0)
  {
    frame_id = 1;
  }

  command_.set_frame_id(frame_id);

  for(int i = 0; i < command_.actuators_size(); ++i)
  {
    command_.mutable_actuators(i)->set_command_id(frame_id);
  }

  feedback_ = base_cyclic_->Refresh(command_);
}

std::vector<double> RobotDriverKortex::getActualQ()
{
  std::vector<double> positions;
  positions.reserve(static_cast<std::size_t>(feedback_.actuators_size()));

  for(const auto & actuator : feedback_.actuators())
  {
    positions.push_back(degToRad(actuator.position()));
  }

  return positions;
}

std::vector<double> RobotDriverKortex::getActualQd()
{
  std::vector<double> velocities;
  velocities.reserve(static_cast<std::size_t>(feedback_.actuators_size()));

  for(const auto & actuator : feedback_.actuators())
  {
    velocities.push_back(degToRad(actuator.velocity()));
  }

  return velocities;
}

std::vector<double> RobotDriverKortex::getJointTorques()
{
  std::vector<double> torques;
  torques.reserve(static_cast<std::size_t>(feedback_.actuators_size()));

  for(const auto & actuator : feedback_.actuators())
  {
    torques.push_back(actuator.torque());
  }

  return torques;
}

void RobotDriverKortex::servoJ(const std::vector<double> & q)
{
  validateCommandSize(q);

  for(std::size_t i = 0; i < actuator_count_; ++i)
  {
    auto * actuator = command_.mutable_actuators(static_cast<int>(i));

    actuator->set_position(static_cast<float>(radToDeg(q[i])));
  }
}

void RobotDriverKortex::speedJ(const std::vector<double> & velocity)
{
  validateCommandSize(velocity);

  for(std::size_t i = 0; i < actuator_count_; ++i)
  {
    command_.mutable_actuators(static_cast<int>(i))->set_velocity(static_cast<float>(radToDeg(velocity[i])));
  }
}

void RobotDriverKortex::tauJ(const std::vector<double> & torque)
{
  validateCommandSize(torque);

  for(std::size_t i = 0; i < actuator_count_; ++i)
  {
    command_.mutable_actuators(static_cast<int>(i))->set_torque_joint(static_cast<float>(torque[i]));
  }
}

// TODO: freeDrive
bool RobotDriverKortex::freeDrive(bool enable)
{
  return false;
}

// TODO: connecting using provided TCP or UDP
void RobotDriverKortex::connect()
{
  auto error_callback = [](k_api::KError error)
  { fmt::print(stderr, "[RobotDriverKortex] Router error: {}\n", error.toString()); };

  // TCP connection for Base and ActuatorConfig services.
  tcp_transport_ = std::make_unique<k_api::TransportClientTcp>();
  tcp_router_ = std::make_unique<k_api::RouterClient>(tcp_transport_.get(), error_callback);
  tcp_transport_->connect(ip_, tcp_port_);

  // UDP connection for the 1 kHz BaseCyclic service.
  udp_transport_ = std::make_unique<k_api::TransportClientUdp>();
  udp_router_ = std::make_unique<k_api::RouterClient>(udp_transport_.get(), error_callback);
  udp_transport_->connect(ip_, udp_port_);

  // Set session data connection information
  auto session_info = k_api::Session::CreateSessionInfo();

  session_info.set_username("admin");
  session_info.set_password("admin");
  session_info.set_session_inactivity_timeout(60000);   // (milliseconds)
  session_info.set_connection_inactivity_timeout(2000); // (milliseconds)

  tcp_session_ = std::make_unique<k_api::SessionManager>(tcp_router_.get());
  tcp_session_->CreateSession(session_info);

  udp_session_ = std::make_unique<k_api::SessionManager>(udp_router_.get());
  udp_session_->CreateSession(session_info);

  // API services.
  base_ = std::make_unique<k_api::Base::BaseClient>(tcp_router_.get());
  base_cyclic_ = std::make_unique<k_api::BaseCyclic::BaseCyclicClient>(udp_router_.get());

  actuator_config_ = std::make_unique<k_api::ActuatorConfig::ActuatorConfigClient>(tcp_router_.get());

  // Start from the normal high-level mode.
  auto servoing_mode = k_api::Base::ServoingModeInformation();

  servoing_mode.set_servoing_mode(k_api::Base::ServoingMode::SINGLE_LEVEL_SERVOING);

  base_->SetServoingMode(servoing_mode);

  actuator_count_ = static_cast<std::size_t>(base_->GetActuatorCount().count());

  if(actuator_count_ == 0)
  {
    throw std::runtime_error("[RobotDriverKortex] Robot reported zero actuators");
  }

  fmt::print("[RobotDriverKortex] Robot has {} actuators\n", actuator_count_);

  initializeCyclicCommand();

  // Cyclic commands require low-level servoing mode.
  servoing_mode.set_servoing_mode(k_api::Base::ServoingMode::LOW_LEVEL_SERVOING);
  base_->SetServoingMode(servoing_mode);
}

void RobotDriverKortex::disconnect() noexcept
{
  command_initialized_ = false;

  // Restore safe/default actuator and base modes before disconnecting.
  try
  {
    if(actuator_config_ && actuator_count_ > 0)
    {
      auto control_mode = k_api::ActuatorConfig::ControlModeInformation();

      control_mode.set_control_mode(k_api::ActuatorConfig::ControlMode::POSITION);

      for(std::size_t i = 0; i < actuator_count_; ++i)
      {
        actuator_config_->SetControlMode(control_mode, static_cast<int>(i + 1));
      }
    }

    if(base_)
    {
      auto servoing_mode = k_api::Base::ServoingModeInformation();

      servoing_mode.set_servoing_mode(k_api::Base::ServoingMode::SINGLE_LEVEL_SERVOING);

      base_->SetServoingMode(servoing_mode);
    }
  }
  catch(const std::exception & error)
  {
    fmt::print(stderr, "[RobotDriverKortex] Failed to restore position/single-level mode: {}\n", error.what());
  }
  catch(...)
  {
    fmt::print(stderr, "[RobotDriverKortex] Unknown error while restoring safe control mode\n");
  }

  // Destroy clients before their routers and transports.
  actuator_config_.reset();
  base_cyclic_.reset();
  base_.reset();

  try
  {
    if(udp_session_)
    {
      udp_session_->CloseSession();
    }
  }
  catch(const std::exception & error)
  {
    std::cerr << "[RobotDriverKortex] Failed to close UDP session: " << error.what() << '\n';
    fmt::print(stderr, "[RobotDriverKortex] Failed to close UDP session: {}\n", error.what());
  }
  catch(...)
  {
    // Ignore
  }

  try
  {
    if(tcp_session_)
    {
      tcp_session_->CloseSession();
    }
  }
  catch(const std::exception & error)
  {
    fmt::print(stderr, "[RobotDriverKortex] Failed to close TCP session: {}\n", error.what());
  }
  catch(...)
  {
    // Ignore
  }

  udp_session_.reset();
  tcp_session_.reset();

  try
  {
    if(udp_router_)
    {
      udp_router_->SetActivationStatus(false);
    }
  }
  catch(...)
  {
    // Ignore
  }

  try
  {
    if(tcp_router_)
    {
      tcp_router_->SetActivationStatus(false);
    }
  }
  catch(...)
  {
    // Ignore
  }

  try
  {
    if(udp_transport_)
    {
      udp_transport_->disconnect();
    }
  }
  catch(...)
  {
    // Ignore
  }

  try
  {
    if(tcp_transport_)
    {
      tcp_transport_->disconnect();
    }
  }
  catch(...)
  {
    // Ignore
  }

  udp_router_.reset();
  tcp_router_.reset();

  udp_transport_.reset();
  tcp_transport_.reset();

  actuator_count_ = 0;
  command_.Clear();
  feedback_.Clear();
}

void RobotDriverKortex::initializeCyclicCommand()
{
  feedback_ = base_cyclic_->RefreshFeedback();

  actuator_count_ = static_cast<std::size_t>(feedback_.actuators_size());

  command_.Clear();

  for(std::size_t i = 0; i < actuator_count_; ++i)
  {
    auto * actuator_command = command_.add_actuators();

    actuator_command->set_position(feedback_.actuators(static_cast<int>(i)).position());
  }

  command_initialized_ = true;
}

void RobotDriverKortex::validateCommandSize(const std::vector<double> & values) const
{
  if(values.size() != actuator_count_)
  {
    throw std::invalid_argument("[RobotDriverKortex] Command size " + std::to_string(values.size())
                                + " does not match actuator count " + std::to_string(actuator_count_));
  }
}

} // namespace kortex_driver

extern "C"
{
  void MC_RTC_ROBOT_DRIVER(std::vector<std::string> & classes)
  {
    classes.push_back("RobotDriverKortex");
  }

  mc_robot_interface::RobotDriver * create(const std::string &, const std::string & ip, const uint16_t & port)
  {
    return new kortex_driver::RobotDriverKortex(ip, port == 0 ? 10000 : port);
  }

  void destroy(mc_robot_interface::RobotDriver * driver)
  {
    delete driver;
  }
}

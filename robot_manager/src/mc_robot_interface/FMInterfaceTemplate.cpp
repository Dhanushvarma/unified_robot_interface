#include <mc_robot_interface/FMInterfaceTemplate.h>
#include <mc_robot_manager/Logging.h>
#include <robot_comm/CommunicationZenoh.h>

#include <mc_control/mc_global_controller.h>

namespace mc_interface_template
{

namespace
{
mc_robot::ControlMode parseModeString(const std::string & mode)
{
  if(mode == "velocity") return mc_robot::VELOCITY;
  if(mode == "torque") return mc_robot::TORQUE;
  return mc_robot::POSITION;
}
} // namespace

FMInterfaceTemplate::FMInterfaceTemplate(const std::string & name,
                                         const mc_rtc::Configuration & config,
                                         uint8_t buffer_size)
: RobotInterfaceBase(name, config, (buffer_size == 0) ? 6 : buffer_size)
{
  mc_fleet::log::info("FMInterfaceTemplate '{}' starting", name);

  const std::string mode = config("controller")("mode", std::string{"position"});
  control_mode_ = parseModeString(mode);
  mc_fleet::log::info("FMInterfaceTemplate '{}' control mode: {}", name, mode);

  mc_rtc::Configuration com_config(config("network_interface"));
  auto comm = robot_comm::CommunicationFactory::makeCommunication(name, com_config);
  comm->setupServer();
  setCommunication(std::move(comm));

  mc_fleet::log::info("FMInterfaceTemplate '{}' ready", name);
}

void FMInterfaceTemplate::updateSensors(robot_controller::Controller & gc)
{
  auto rx = communication().receive();
  if(!rx || rx->empty()) return;

  auto s = communication().serializer()->deserialize<robot_comm::State>(robot_comm::MessageType::STATE, rx->data(),
                                                                        rx->size());
  if(!s) return;

  if(!s->position.empty()) gc.setEncoderValues(name(), s->position);
  if(!s->velocity.empty()) gc.setEncoderVelocities(name(), s->velocity);
  if(!s->torque.empty()) gc.setJointTorques(name(), s->torque);

  if(!gc_initialized_ && !s->position.empty())
  {
    gc.initialize(s->position);
    gc_initialized_ = true;
    mc_fleet::log::info("[FMInterfaceTemplate] '{}' controller initialized", name());
  }
}

void FMInterfaceTemplate::updateControl(robot_controller::Controller & gc)
{
  if(!gc_initialized_) return;

  robot_comm::Command command;

  switch(control_mode_)
  {
    case mc_robot::POSITION:
      command.position = gc.command(name(), robot_controller::ControlMode::POSITION);
      break;

    case mc_robot::VELOCITY:
      command.velocity = gc.command(name(), robot_controller::ControlMode::VELOCITY);
      break;

    case mc_robot::TORQUE:
      command.torque = gc.command(name(), robot_controller::ControlMode::TORQUE);
      break;
  }

  if(!communication().send(communication().encode(command)))
    mc_fleet::log::warning("[FMInterfaceTemplate] '{}' failed to send command", name());
}

} // namespace mc_interface_template

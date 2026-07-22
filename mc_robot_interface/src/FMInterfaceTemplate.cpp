#include <mc_robot_interface/FMInterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <random>

namespace mc_interface_template
{

FMInterfaceTemplate::FMInterfaceTemplate(const std::string & name, const mc_rtc::Configuration & config)
: RobotInterfaceBase(name, config)
{
  mc_rtc::log::success("FMInterfaceTemplate manager start");

  mc_rtc::Configuration com_config(config("network_interface"));
  setCommunication(mc_communication::CommunicationFactory::makeCommunication("server", com_config));

  // Subscribe to state messages — callback updates the latest state
  auto state_sub =
      communication().subscribe<mc_communication::State>("state",
                                                         [this](const mc_communication::State & s)
                                                         {
                                                           setState(s);
                                                           mc_rtc::log::success("Received state from {}", this->name());
                                                         });

  if(!state_sub)
  {
    mc_rtc::log::error("Failed to subscribe to state for {}", this->name());
  }

  mc_rtc::log::info("FMInterfaceTemplate done");
}

void FMInterfaceTemplate::updateSensors()
{
  // State is received asynchronously via the subscriber callback.
  // If there is complicated execution everytime a state is received, do that there.
}

void FMInterfaceTemplate::updateControl()
{
  static std::mt19937 rng{std::random_device{}()};
  static std::uniform_real_distribution<double> dist(-1.0, 1.0);

  constexpr size_t dof = 6;

  mc_communication::Command command;

  command.kp = dist(rng);
  command.kd = dist(rng);

  command.position.resize(dof);
  command.velocity.resize(dof);
  command.torque.resize(dof);

  for(size_t i = 0; i < dof; ++i)
  {
    command.position[i] = dist(rng);
    command.velocity[i] = dist(rng);
    command.torque[i] = dist(rng);
  }

  setCommand(command);

  // Publish command
  bool sent = communication().publish("command", command);

  if(sent)
  {
    mc_rtc::log::success("Sent command {}", command.kp);
  }
  else
  {
    mc_rtc::log::warning("Failed to send COMMAND to robot {}", name());
  }
}

} // namespace mc_interface_template

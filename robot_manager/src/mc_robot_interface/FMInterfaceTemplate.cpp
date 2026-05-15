#include <mc_communication/CommunicationZenoh.h>
#include <mc_robot_interface/FMInterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <random>
#include <thread>

namespace mc_interface_template
{

FMInterfaceTemplate::FMInterfaceTemplate(const std::string & name,
                                         const mc_rtc::Configuration & config,
                                         uint8_t buffer_size)
: RobotInterfaceBase(name, config, (buffer_size == 0) ? 6 : buffer_size)
{
  mc_rtc::log::success("FMInterfaceTemplate manager start");

  mc_rtc::Configuration com_config(config("communication"));
  setCommunication(mc_communication::CommunicationFactory::makeCommunicationSever(name, com_config));

  mc_rtc::log::info("FMInterfaceTemplate done");
};

void FMInterfaceTemplate::updateSensors()
{
  if(communication().receiveMessage(mc_communication::Communication::MessageType::STATE))
  {
    auto latest_state = communication().latestState();

    // TODO: delete logging
    communication().states().push_back(latest_state);

    mc_rtc::log::success("Received state");
  }
  else
  {
    mc_rtc::log::error("Trouble receiving state");
  }
};

void FMInterfaceTemplate::updateControl()
{
  static std::mt19937 rng{std::random_device{}()};
  static std::uniform_real_distribution<double> dist(-1.0, 1.0);

  mc_communication::Command command;

  constexpr size_t dof = 6;

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

  // Serialize to FlatBuffers
  auto buffer = mc_communication::Communication::serializeCommand(command);

  // Send to robot manager
  bool sent =
      communication().sendMessage(mc_communication::Communication::MessageType::COMMAND, buffer.data(), buffer.size());

  // TODO: delete logging
  communication().commands().push_back(command);

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

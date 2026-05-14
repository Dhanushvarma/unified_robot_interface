#include <mc_communication/CommunicationZenoh.h>
#include <mc_robot_interface/InterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <random>
#include <thread>

namespace mc_interface_template
{

InterfaceTemplate::InterfaceTemplate(const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("InterfaceTemplate remote start");

  mc_rtc::Configuration com_config("local_robot/etc/communication.yaml");
  if(com_config.has("name"))
  {
    setCommunication(mc_communication::CommunicationFactory::makeCommunicationClient(com_config));
  }
  else
  {
    mc_rtc::log::error_and_throw("Missing name of robot");
  }

  bool got_config = false;

  while(!got_config && !interrupt)
  {
    mc_rtc::log::info("[mc_communication] Waiting for config from robot manager");
    got_config = communication().receiveMessage(mc_communication::Communication::MessageType::CONFIG);
    if(!got_config)
    {
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  }

  if(interrupt)
  {
    mc_rtc::log::warning("Initialization interrupted");
    return;
  }

  if(!communication().latestConfig().empty())
  {
    mc_rtc::log::success("HERE IS CONFIG");
    mc_rtc::log::info(communication().latestConfig().dump(true, true));
  }
  mc_rtc::log::info("InterfaceTemplate local done");
};

void InterfaceTemplate::updateSensors()
{
  static std::mt19937 rng{std::random_device{}()};
  static std::uniform_real_distribution<double> dist(-1.0, 1.0);

  mc_communication::State state;

  constexpr size_t dof = 6; // example: 6 joints

  state.position.resize(dof);
  state.velocity.resize(dof);
  state.torque.resize(dof);

  for(size_t i = 0; i < dof; ++i)
  {
    state.position[i] = dist(rng);
    state.velocity[i] = dist(rng);
    state.torque[i] = dist(rng);
  }

  // Serialize to FlatBuffers
  auto buffer = mc_communication::Communication::serializeState(state);

  // Send to robot manager
  bool sent =
      communication().sendMessage(mc_communication::Communication::MessageType::STATE, buffer.data(), buffer.size());

  if(!sent)
  {
    mc_rtc::log::warning("Failed to send STATE to robot manager");
  }
}

void InterfaceTemplate::updateControl() {
  // TODO: receive commands from manager
};

} // namespace mc_interface_template

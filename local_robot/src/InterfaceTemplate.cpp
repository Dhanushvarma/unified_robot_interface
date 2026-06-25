#include <mc_communication/CommunicationZenoh.h>
#include <InterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <filesystem>
#include <fstream>
#include <random>
#include <thread>

namespace mc_interface_template
{

// Helper function to find config file
static std::string findConfigFile()
{
  // Try installed location first
  const char * install_prefix = CMAKE_INSTALL_PREFIX;
  std::filesystem::path installed_config = std::string(install_prefix) + "/etc/communication.yaml";

  if(std::filesystem::exists(installed_config))
  {
    return installed_config.string();
  }

  // Fall back to source tree
  std::filesystem::path source_config = std::string(PROJECT_SOURCE_DIR) + "/etc/communication.yaml";
  if(std::filesystem::exists(source_config))
  {
    return source_config.string();
  }

  // If neither exists, return source path anyway (will error with helpful message)
  return source_config.string();
}

InterfaceTemplate::InterfaceTemplate(const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("InterfaceTemplate remote start");

  mc_rtc::Configuration com_config("/home/tduvinage/devel/sandbox/mc_rtc_interface/local_robot/etc/communication.yaml");
  if(com_config.has("name"))
  {
    setCommunication(mc_communication::CommunicationFactory::makeCommunicationClient(com_config));
  }
  else
  {
    mc_rtc::log::error_and_throw("Missing name of robot");
  }

  mc_communication::ByteBuffer config_;

  while(!config_.empty() && !interrupt)
  {
    mc_rtc::log::info("[mc_communication] Waiting for config from robot manager");
    config_ = communication().receive().value();
    if(config_.empty())
    {
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  }

  if(interrupt)
  {
    mc_rtc::log::warning("Initialization interrupted");
    return;
  }

  mc_rtc::log::success("HERE IS CONFIG");
  mc_rtc::log::info(config_);

  mc_rtc::log::info("InterfaceTemplate local done");
};

void InterfaceTemplate::updateSensors()
{
  static std::mt19937 rng{std::random_device{}()};
  static std::uniform_real_distribution<double> dist(-1.0, 1.0);

  constexpr size_t dof = 6;

  mc_communication::State state;

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
  auto buffer = communication().encode(state);

  // Send to robot manager
  bool sent = communication().send(buffer);

  if(sent)
  {
    mc_rtc::log::success("Sent state");
  }
  else
  {
    mc_rtc::log::warning("Failed to send STATE to robot manager");
  }
}

void InterfaceTemplate::updateControl()
{
  if(auto latest_command = communication().receive())
  {
    if(!latest_command->empty())
    {
      mc_rtc::log::success("Received command");
    }
  }
  else
  {
    mc_rtc::log::error("Trouble receiving command");
  }
};

} // namespace mc_interface_template

// #include <mc_communication/Communication.h>
// #include <mc_communication/CommunicationFactory.h>
// #include <InterfaceTemplate.h>

// #include <mc_rtc/logging.h>

// #include <filesystem>
// #include <random>
// #include <thread>

// namespace mc_interface_template
// {

// // Helper function to find config file
// static std::string findConfigFile()
// {
//   // Try installed location first
//   const char * install_prefix = CMAKE_INSTALL_PREFIX;
//   std::filesystem::path installed_config = std::string(install_prefix) + "/etc/communication.yaml";

//   if(std::filesystem::exists(installed_config))
//   {
//     return installed_config.string();
//   }

//   // Fall back to source tree
//   std::filesystem::path source_config = std::string(PROJECT_SOURCE_DIR) + "/etc/communication.yaml";
//   if(std::filesystem::exists(source_config))
//   {
//     return source_config.string();
//   }

//   return source_config.string();
// }

// InterfaceTemplate::InterfaceTemplate(const std::atomic<bool> & interrupt)
// {
//   mc_rtc::log::success("InterfaceTemplate remote start");

//   mc_rtc::Configuration com_config(findConfigFile());
//   // mc_rtc::Configuration com_config("local_robot/etc/communication.yaml");
//   // mc_rtc::Configuration
//   // com_config("/home/tduvinage/devel/sandbox/mc_rtc_interface/local_robot/etc/communication.yaml");

//   if(!com_config.has("name"))
//   {
//     mc_rtc::log::error_and_throw("Missing name of robot");
//   }

//   setCommunication(mc_communication::CommunicationFactory::makeCommunication("client", com_config));

//   // ── Query for config from robot manager (blocking until we get one) ──
//   while(!interrupt)
//   {
//     auto config = communication().query<std::string>(name() + "/config", std::chrono::seconds(2));

//     if(config)
//     {
//       mc_rtc::log::success("[mc_communication] Got config from server");
//       mc_rtc::log::info(*config);
//       // TODO: parse *config here if needed and apply it
//       break;
//     }

//     mc_rtc::log::info("[mc_communication] Waiting for config...");
//   }

//   if(interrupt)
//   {
//     mc_rtc::log::warning("Initialization interrupted");
//     return;
//   }

//   // ── Subscribe to command from robot manager ──
//   communication().subscribe<mc_communication::Command>("command",
//                                                        [this](const mc_communication::Command & cmd)
//                                                        {
//                                                          setCommand(cmd);
//                                                          mc_rtc::log::success("Received command kp={:.3f}", cmd.kp);
//                                                        });

//   mc_rtc::log::info("InterfaceTemplate local done");
// }

// void InterfaceTemplate::updateSensors()
// {
//   static std::mt19937 rng{std::random_device{}()};
//   static std::uniform_real_distribution<double> dist(-1.0, 1.0);

//   constexpr size_t dof = 6;

//   mc_communication::State state;
//   state.position.resize(dof);
//   state.velocity.resize(dof);
//   state.torque.resize(dof);

//   for(size_t i = 0; i < dof; ++i)
//   {
//     state.position[i] = dist(rng);
//     state.velocity[i] = dist(rng);
//     state.torque[i] = dist(rng);
//   }

//   setState(state);

//   bool sent = communication().publish("state", state);

//   if(sent)
//   {
//     mc_rtc::log::success("Sent state");
//   }
//   else
//   {
//     mc_rtc::log::warning("Failed to send STATE to robot manager");
//   }
// }

// void InterfaceTemplate::updateControl()
// {
//   // Commands arrive asynchronously via the subscribe callback.
//   // The latest command is stored via setCommand() — access via command().
// }

// } // namespace mc_interface_template

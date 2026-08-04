#include <robot_interface/RobotInterface.h>

#include <mc_rtc/Configuration.h>
#include <mc_rtc/logging.h>

#include <CLI/CLI.hpp>

#include <atomic>
#include <csignal>

static std::atomic<bool> g_interrupt{false};

static void signal_handler(int /*sig*/)
{
  g_interrupt = true;
}

int main(int argc, char ** argv)
{
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  CLI::App app{"Robot interface process"};

  std::string config_path;
  std::string name;

  app.add_option("-c,--config", config_path, "Path to robot interface config file (YAML)")->required();
  app.add_option("-n,--name", name, "Robot name (overrides 'name' in config)");

  CLI11_PARSE(app, argc, argv);

  mc_rtc::Configuration config(config_path);

  if(name.empty())
  {
    if(config.has("name"))
      name = static_cast<std::string>(config("name"));
    else
      mc_rtc::log::error_and_throw("'name' must be provided via --name or in the config file");
  }

  mc_rtc::Configuration comm_config = config("network_interface");

  mc_rtc::log::info("[robot_interface] Starting '{}' from config '{}'", name, config_path);

  mc_robot_interface::RobotInterface iface(name, comm_config);
  iface.run(g_interrupt);

  mc_rtc::log::info("[robot_interface] '{}' exited cleanly", name);
  return 0;
}

#include <mc_robot_manager/RobotManager.h>
#include <robot_controller/Controller.h>

#include <mc_control/Configuration.h>
// #include "zenoh.hxx"

#include <CLI/CLI.hpp>

#include <fmt/core.h>
#include <sys/shm.h>

#include <iostream>

namespace mc_fleet
{

void run(void * data, const std::atomic<bool> & interrupt)
{
  fmt::print("fleet run start\n");
  auto * robot_manager = static_cast<RobotManager *>(data);
  robot_controller::Controller & controller{robot_manager->controller()};

  while(controller.isRunning() && !interrupt)
  {
    robot_manager->notify();
    sched_yield();
  }

  fmt::print("fleet run done\n");
}

void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt)
{
  fmt::print("fleet init start\n");

  CLI::App app{"MCFleetControl options"};

  std::string mc_config_path;
  // TODO: remove `required()`
  // There should be a default path where RobotManager looks in first
  app.add_option("-f,--config", mc_config_path, "Path to mc_rtc configuration file")->required();
  app.footer("see etc/mc_rtc.yaml for example configuration");

  try
  {
    app.parse(argc, argv);
  }
  catch(const CLI::ParseError & e)
  {
    std::exit(app.exit(e));
  }

  fmt::print("mc_fleet::init 2\n");

  /* Initialize robot manager */
  std::unique_ptr<RobotManager> robot_manager;
  robot_manager = std::make_unique<RobotManager>(mc_config_path, interrupt);

  fmt::print("fleet init done\n");

  return robot_manager.release();
}

} // namespace mc_fleet

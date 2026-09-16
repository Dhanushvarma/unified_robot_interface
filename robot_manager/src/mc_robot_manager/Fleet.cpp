#include <mc_robot_manager/RobotManager.h>
#include <robot_controller/Controller.h>

#include <mc_control/Configuration.h>
// #include "zenoh.hxx"

#include <CLI/CLI.hpp>
#include <sys/shm.h>

#include <iostream>

namespace robot_manager
{

void run(void * data, const std::atomic<bool> & interrupt)
{
  auto * robot_manager = static_cast<RobotManager *>(data);
  robot_controller::Controller & controller{robot_manager->controller()};

  while(controller.isRunning() && !interrupt)
  {
    robot_manager->notify();
    sched_yield();
  }
}

void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt)
{
  CLI::App app{"uri options"};

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

  /* Initialize robot manager */
  std::unique_ptr<RobotManager> robot_manager;
  robot_manager = std::make_unique<RobotManager>(mc_config_path, interrupt);

  return robot_manager.release();
}

} // namespace robot_manager

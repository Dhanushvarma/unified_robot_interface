#include <mc_robot_manager/RobotManager.h>
#include <robot_controller/Controller.h>

#include <mc_control/Configuration.h>
// #include "zenoh.hxx"

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

void * init(const std::string & mc_config_path, uint64_t & /*cycle_ns*/, const std::atomic<bool> & interrupt)
{
  /* Initialize robot manager */
  std::unique_ptr<RobotManager> robot_manager;
  robot_manager = std::make_unique<RobotManager>(mc_config_path, interrupt);

  return robot_manager.release();
}

} // namespace robot_manager

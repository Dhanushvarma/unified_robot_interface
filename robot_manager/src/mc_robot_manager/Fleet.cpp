#include <mc_robot_manager/RobotManager.h>
#include <robot_controller/Controller.h>

#include <mc_control/Configuration.h>
// #include "zenoh.hxx"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

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

  std::string mc_config_path;
  std::string com_config_path;
  po::options_description desc("MCFleetControl options");
  // clang-format off
  desc.add_options()
    ("help,h", "Display help message")
    ("config,f", po::value<std::string>(&mc_config_path), "Path to mc_rtc configuration file");
  // clang-format on

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if(vm.count("help") != 0U)
  {
    std::cout << desc << "\n";
    std::cout << "see etc/mc_rtc.yaml for example configuration\n";
    std::exit(0);
  }

  fmt::print("mc_fleet::init 2\n");

  /* Initialize robot manager */
  std::unique_ptr<RobotManager> robot_manager;
  robot_manager = std::make_unique<RobotManager>(mc_config_path, interrupt);

  fmt::print("fleet init done\n");

  return robot_manager.release();
}

} // namespace mc_fleet

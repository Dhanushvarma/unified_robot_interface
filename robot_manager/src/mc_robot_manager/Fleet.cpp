#include <mc_robot_manager/RobotManager.h>

#include <mc_control/Configuration.h>
#include <mc_control/mc_global_controller.h>
#include <mc_rtc/logging.h>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <sys/shm.h>

namespace mc_fleet
{

void run(void * data, const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("fleet run start");
  auto * robot_manager = static_cast<RobotManager *>(data);
  mc_control::MCGlobalController & gcontroller{robot_manager->gcontroller()};

  while(gcontroller.running && !interrupt)
  {
    robot_manager->notify();
    sched_yield();
  }

  // TODO: delete logging
  for(const auto & [name, interface] : robot_manager->interfaces())
  {
    std::string fn = name + "_log.json";
    interface->dumpLog(fn);
  }

  mc_rtc::log::info("fleet run done");
}

void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("fleet init start");

  std::string conf_file;
  po::options_description desc("MCFleetControl options");
  // clang-format off
   desc.add_options()
    ("help,h", "Display help message")
    ("conf,f", po::value<std::string>(&conf_file), "Configuration file");
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

  mc_rtc::log::info("mc_fleet::init 2");

  /* Initialize robot manager */
  mc_control::MCGlobalController::GlobalConfiguration gconfig(conf_file, nullptr);
  auto robot_manager = std::make_unique<RobotManager>(gconfig, interrupt);

  mc_rtc::log::info("fleet init done");

  return robot_manager.release();
}

} // namespace mc_fleet

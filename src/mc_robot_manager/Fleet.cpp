#include <mc_robot_manager/Fleet.h>
#include <mc_robot_manager/RobotManager.h>

#include <mc_control/Configuration.h>
#include <mc_control/mc_global_controller.h>
#include <mc_rtc/logging.h>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>

namespace mc_fleet
{

void run(void * data, const std::atomic<bool> & interrupt)
{
  mc_rtc::log::info("mc_fleet::run");
}

void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt)
{
  mc_rtc::log::info("mc_fleet::init 1");

  std::string conf_file;
  std::string testing_string;
  po::options_description desc("MCFleetControl options");
  // clang-format off
   desc.add_options()
    ("help,h", "Display help message")
    ("conf,f", po::value<std::string>(&conf_file), "Configuration file")
    ("test,t", po::value<std::string>(&testing_string), "Configuration file");
  // clang-format on

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if(vm.count("help") != 0U)
  {
    std::cout << desc << "\n";
    std::cout << "see etc/mc_rtc.yaml for example configuration\n";
    return nullptr;
  }

  mc_rtc::log::info("mc_fleet::init 2");

  /* Initialize robot manager */
  mc_control::MCGlobalController::GlobalConfiguration gconfig(conf_file, nullptr);
  auto * robot_manager = new RobotManager(gconfig);

  mc_rtc::log::info("mc_fleet::init 5");

  mc_rtc::log::info("----------------------------------------------------------------------------");
  mc_rtc::log::info("GLOBAL CONFIG");
  mc_rtc::log::info(gconfig.config("Robots").dump(true, true));

  mc_rtc::log::info("mc_fleet::init 6");

  // TODO: start network - server

  static int dummy_success_flag = 42;
  return &dummy_success_flag;
}

} // namespace mc_fleet

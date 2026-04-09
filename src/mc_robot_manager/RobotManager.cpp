#include <mc_rtc/logging.h>
#include <mc_robot_manager/RobotManager.h>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

namespace mc_fleet
{

struct DefaultConfig
{
  std::string module{};
  std::string control_mode{};
  std::string driver{};
  std::string control_freq{};
  std::string network_protocol{};
};

void printConfig(const std::string & name, const mc_control::Configuration & config)
{
  mc_rtc::log::info(name);
  mc_rtc::log::info(config.dump(true, true));
}

void * globalInit(mc_control::MCGlobalController::GlobalConfiguration & gconfig, const std::atomic<bool> & interrupt)
{

  auto global_controller = std::make_unique<mc_control::MCGlobalController>(gconfig);
  auto threads = std::make_unique<std::vector<std::thread>>();

  static int dummy_success_flag = 42;
  return &dummy_success_flag;
}

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

  // Process config file
  mc_control::MCGlobalController::GlobalConfiguration gconfig(conf_file, nullptr);
  if(!gconfig.config.has("Robots"))
  {
    mc_rtc::log::error_and_throw<std::runtime_error>(
        "No `Robots` section in the configuration, see etc/mc_rtc.yaml for an example");
  }

  mc_rtc::log::info("mc_fleet::init 3");

  DefaultConfig default_config{};
  if(gconfig.config.has("Default"))
  {
    mc_control::Configuration dc = gconfig.config("Default");
    default_config.module = dc("module", std::string(""));
    default_config.control_mode = dc("control_mode", std::string("position"));
    default_config.driver = dc("driver", std::string(""));
    // TODO: add default control_freq
    default_config.control_freq = dc("control_freq", std::string(""));
    default_config.network_protocol = dc("network_protocol", std::string("tcp"));
  }

  mc_rtc::log::info("mc_fleet::init 4");

  mc_control::Configuration robots_config = gconfig.config("Robots");
  for(auto & robot_name : robots_config.keys())
  {
    mc_control::Configuration robot_config{gconfig.config("Robots")(robot_name)};

    if(robot_config.has("base"))
    {
      mc_control::Configuration base_config{};
      base_config.load(robots_config(robot_config("base")));
      base_config.load(robot_config);
      robot_config.load(base_config);
    }

    if(!robot_config.has("module"))
    {
      robot_config.add("module", default_config.module);
    }

    if(!robot_config.has("controller"))
    {
      robot_config.add("controller");
      robot_config("controller").add("mode", default_config.control_mode);
      robot_config("controller").add("driver", default_config.driver);
      robot_config("controller").add("freq", default_config.control_freq);
    }
    else
    {
      if(!robot_config("controller").has("mode"))
      {
        robot_config("controller").add("mode", default_config.control_mode);
      }
      if(!robot_config("controller").has("driver"))
      {
        robot_config("controller").add("driver", default_config.driver);
      }
      if(!robot_config("controller").has("freq"))
      {
        robot_config("controller").add("freq", default_config.control_freq);
      }
    }

    if(robot_config.has("network"))
    {
      if(!robot_config("network").has("protocol"))
      {
        robot_config("network").add("protocol", default_config.control_freq);
      }
    }
    else
    {
      mc_rtc::log::error_and_throw("No `network` section in the configuration of robot {}", robot_name);
    }
  }

  mc_rtc::log::info("mc_fleet::init 5");

  printConfig("global_config", gconfig.config("Robots"));

  mc_rtc::log::info("mc_fleet::init 6");

  globalInit(gconfig, interrupt);

  static int dummy_success_flag = 42;
  return &dummy_success_flag;
}

} // namespace mc_fleet

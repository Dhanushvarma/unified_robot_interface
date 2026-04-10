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
  double time_step{};
  std::string network_protocol{};
};

void * globalInit(mc_control::MCGlobalController::GlobalConfiguration & gconfig, const std::atomic<bool> & interrupt)
{
  std::cout << "globalInit 1" << std::endl;

  auto controller = std::make_unique<mc_control::MCGlobalController>(gconfig);
  auto threads = std::make_unique<std::vector<std::thread>>();

  std::cout << "globalInit 2" << std::endl;

  const mc_control::Configuration robots_config{gconfig.config("Robots")};

  /* checkTimeStep*/
  // TODO: at what time step mc_rtc should be running at? compare to max_dt or min_dt
  double controller_dt = controller->controller().timeStep;
  std::vector<double> dts{};
  double max_dt{0};
  for(auto & robot_name : robots_config.keys())
  {
    const mc_control::Configuration robot_config{robots_config(robot_name)};
    if(robot_config("controller").has("time_step"))
    {
      std::cout << "globalInit 2.1" << std::endl;
      const double t{robot_config("controller")("time_step")};
      std::cout << "globalInit 2.1.1" << std::endl;
      dts.push_back(t);
    }
    else
    {
      std::cout << "globalInit 2.2" << std::endl;
      robot_config("controller").add("time_step", controller->timestep());
      const double t{static_cast<double>(robot_config("controller")("time_step"))};
      dts.push_back(t);
    }
  }

  std::cout << "globalInit 3" << std::endl;

  auto controller_dt_ns = static_cast<size_t>(controller_dt * 1e9);
  for(auto & dt : dts)
  {
    if(dt > max_dt)
    {
      max_dt = dt;
    }
    auto dt_ns = static_cast<size_t>(dt * 1e9);
    if(controller_dt_ns % dt_ns != 0)
    {
      // TODO: error and throw
      mc_rtc::log::warning("[mc_fleet] mc_rtc time step must be a multiple of the robot's control loop time step "
                           "(RobotTimeStep= {}ms, Timestep={}ms)",
                           dt, controller_dt);
    }
  }

  std::cout << "globalInit 4" << std::endl;

  if(controller_dt < max_dt)
  {
    // TODO: error and throw
    mc_rtc::log::warning("[mc_fleet] mc_rtc should not run faster than the slowest robot's control time step "
                         "(Timestep: {}s, Robot time_step {}s)",
                         controller_dt, max_dt);
  }

  std::cout << "globalInit 5" << std::endl;

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

  /* Process config file */
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
    default_config.time_step = dc("time_step", 0.001);
    default_config.network_protocol = dc("network_protocol", std::string("tcp"));
  }

  mc_rtc::log::info("mc_fleet::init 4");

  mc_control::Configuration robots_config = gconfig.config("Robots");
  mc_robot::RobotInterfaceFactory robot_factory{};
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
      robot_config("controller").add("time_step", default_config.time_step);
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
      if(!robot_config("controller").has("time_step"))
      {
        robot_config("controller").add("time_step", default_config.time_step);
      }
    }

    if(robot_config.has("network"))
    {
      if(!robot_config("network").has("protocol"))
      {
        robot_config("network").add("protocol", default_config.network_protocol);
      }
    }
    else
    {
      mc_rtc::log::error_and_throw("No `network` section in the configuration of robot {}", robot_name);
    }

    robot_factory.addRobotInterface(robot_name, robot_config);
  }

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

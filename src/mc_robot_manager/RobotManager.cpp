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

void printConfig(const std::string & name, const mc_control::Configuration & config)
{
  mc_rtc::log::info(name);
  mc_rtc::log::info(config.dump(true, true));
}

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

  size_t controller_dt_ns = controller_dt * 1e9;
  for(auto & dt : dts)
  {
    if(dt > max_dt) max_dt = dt;
    size_t dt_ns = dt * 1e9;
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

  // size_t n_steps = controller_ns / cycle_ns;
  // size_t freq = std::ceil(1 / controller_s);
  // size_t robot_freq = std::ceil(1 / cycle_s);
  // mc_rtc::log::info(
  //     "[mc_rtde] mc_rtc running at {}Hz, robot running at {}Hz, will compute commands every {} robot control step",
  //     freq, robot_freq, n_steps);

  // auto & robots = controller.controller().robots();
  // // Initialize all real robots
  // for(size_t i = controller.realRobots().size(); i < robots.size(); ++i)
  // {
  //   controller.realRobots().robotCopy(robots.robot(i), robots.robot(i).name());
  // }
  // // Initialize controlled ur robots
  // loop_data->urs = new std::vector<URControlLoopPtr<cm>>();
  // auto & urs = *loop_data->urs;

  // std::vector<std::thread> ur_init_thread;
  // std::mutex ur_init_mutex;
  // std::condition_variable ur_init_cv;
  // bool ur_init_ready = false;
  // for(auto & robot : robots)
  // {
  //   if(robot.mb().nrDof() == 0)
  //   {
  //     continue;
  //   }

  //   if(rtdeConfig.has(robot.name()))
  //   {
  //     auto robotConfig = rtdeConfig(robot.name());
  //     ur_init_thread.emplace_back(
  //         [&, robotConfig]()
  //         {
  //           {
  //             std::unique_lock<std::mutex> lock(ur_init_mutex);
  //             ur_init_cv.wait(lock, [&ur_init_ready]() { return ur_init_ready; });
  //           }
  //           auto ur = std::unique_ptr<URControlLoop<cm>>(new URControlLoop<cm>(robot.name(), robotConfig,
  //           cycle_s)); std::unique_lock<std::mutex> lock(ur_init_mutex); urs.emplace_back(std::move(ur));
  //         });
  //   }
  //   else
  //   {
  //     mc_rtc::log::warning("The loaded controller uses an actuated robot that is not configured and not ignored:
  //     {}",
  //                          robot.name());
  //   }
  // }

  // ur_init_ready = true;
  // ur_init_cv.notify_all();
  // for(auto & th : ur_init_thread)
  // {
  //   th.join();
  // }

  // for(auto & ur : urs)
  // {
  //   ur->init(controller);
  // }

  // controller.init(robots.robot().encoderValues());

  // controller.running = true;
  // controller.controller().gui()->addElement(
  //     {"RTDE"}, mc_rtc::gui::Button("Stop controller", [&controller]() { controller.running = false; }));

  // // Start ur control loop
  // static std::mutex startMutex;
  // static std::condition_variable startCV;
  // static bool startControl = false;

  // for(auto & ur : urs)
  // {
  //   // Create UR control threads but don't run them yet, wait on startCV
  //   // until the first control command has been computed (by MCGlobalController::run)
  //   loop_data->ur_threads_->emplace_back(
  //       [&]() { ur->controlThread(controller, startMutex, startCV, startControl, controller.running); });
  // }

  // // Create main mc_rtc control thread:
  // // - get the latest sensor readings from the robots
  // // - run mc_rtc controller (MCGlobalController::run)
  // // - sends the latest command
  // //
  // // This thread runs at the same frequency as the low-level control (cycle_ns).
  // // However, the mc_rtc control loop can run at a lower frequency.
  // // This frequency must be a multiple (n_steps) of cycle_ns as lower frequencies
  // // are simply achieved by calling MCGlobalController every n_steps iterations of the low-level control loop
  // loop_data->controller_run_ = new std::thread(
  //     [loop_data, n_steps, &interrupt, &robot_count]()
  //     {
  //       auto controller_ptr = loop_data->controller_;
  //       auto & controller = *controller_ptr;
  //       auto & urs_ = *loop_data->urs;
  //       std::mutex controller_run_mtx;
  //       std::timespec tv;
  //       clock_gettime(CLOCK_REALTIME, &tv);
  //       // Current time in milliseconds
  //       double current_t = tv.tv_sec * 1000 + tv.tv_nsec * 1e-6;
  //       // Will record the time that passed between two runs
  //       double elapsed_t = 0;
  //       controller.controller().logger().addLogEntry("mc_rtde_delay", [&elapsed_t]() { return elapsed_t; });

  //       size_t n_iter = n_steps;

  //       while(controller.running)
  //       {
  //         std::unique_lock lck(controller_run_mtx);
  //         loop_data->controller_run_cv_.wait(lck);
  //         if(interrupt)
  //         {
  //           std::cout << "controller_run_ interrupted" << std::endl;
  //           controller.running = false;
  //           return;
  //         }
  //         clock_gettime(CLOCK_REALTIME, &tv);
  //         elapsed_t = tv.tv_sec * 1000 + tv.tv_nsec * 1e-6 - current_t;
  //         current_t = elapsed_t + current_t;

  //         while(true)
  //         {
  //           std::string previous_robot = "";
  //           std::string new_robot = "";
  //           bool switch_needed = false;
  //           {
  //             std::lock_guard<std::mutex> lock(loop_data->signal_mutex);
  //             if(!loop_data->replace_queue.empty())
  //             {
  //               auto request = loop_data->replace_queue.front();
  //               loop_data->replace_queue.pop();

  //               previous_robot = request.old_robot_name;
  //               new_robot = request.new_robot_name;
  //               switch_needed = true;
  //             }
  //           }

  //           // Process the replacement
  //           if(switch_needed)
  //           {
  //             bool switched = false;
  //             for(auto & ur : urs_)
  //             {
  //               std::string active = ur->activeName();
  //               if(active == previous_robot)
  //               {
  //                 mc_rtc::log::info("[mc_rtde] Switching from {} to {}", previous_robot, new_robot);
  //                 ur->setActiveRobot(controller, new_robot, startMutex, startCV, startControl, controller.running);
  //                 switched = true;
  //                 break;
  //               }
  //             }
  //             if(!switched)
  //             {
  //               mc_rtc::log::error("[mc_rtde] Cannot find robot {} ", previous_robot);
  //             }
  //           }
  //           else
  //           {
  //             break;
  //           }
  //         }

  //         // Update from the latest available ur sensors (non blocking)
  //         for(auto & ur : urs_)
  //         {
  //           ur->updateSensors(controller);
  //         }

  //         if(n_iter % n_steps == 0)
  //         {
  //           // Run the controller
  //           // std::cout << "Running the controller at iter " << n_iter << std::endl;
  //           controller.run();
  //         }

  //         // Wait until the first command has been computed to start the robot's control loop
  //         startControl = true;
  //         startCV.notify_all();

  //         // Update ur commands (non blocking)
  //         // If mc_rtc is running at a lower frequency than the low-level control, then intermediate commands
  //         // will need to be interpolated (especially for position control)
  //         for(auto & ur : urs_)
  //         {
  //           ur->updateControl(controller);
  //         }
  //         n_iter++;
  //       }
  //     });

  // return loop_data;

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
    default_config.time_step = dc("time_step", 0.001);
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

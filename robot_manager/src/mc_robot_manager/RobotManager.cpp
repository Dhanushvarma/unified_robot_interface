#include <mc_communication/CommunicationZenoh.h>
#include <mc_rtc/logging.h>
#include <mc_robot_manager/RobotManager.h>

#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace mc_fleet
{

RobotManager::RobotManager() : gconfig_(mc_rtc::Configuration{}) {};

RobotManager::RobotManager(mc_control::MCGlobalController::GlobalConfiguration & gconfig,
                           const std::atomic<bool> & interrupt)
: gconfig_(gconfig)
{
  processGConfig(gconfig_);
  gcontroller_ = std::make_unique<mc_control::MCGlobalController>(gconfig);

  // // Connect to the signal
  // auto & mc_controller = gcontroller_->controller();
  // replace_slot_ = mc_controller.replaceRobot.connect(
  //     [&](const std::string & old_robot_name, const std::string & new_robot_name)
  //     {
  //       std::lock_guard<std::mutex> lock(replace_mutex_);
  //       replace_queue_.push({old_robot_name, new_robot_name});
  //       mc_rtc::log::info("[mc_rtde] Signal caught: Request switching from {} to {}", old_robot_name,
  //       new_robot_name);
  //     });

  init(interrupt);
};

RobotManager::~RobotManager()
{
  gcontroller_->running = false;

  cv_.notify_all();

  {
    std::lock_guard<std::mutex> lock(start_mutex_);
    start_control_ = true;
  }
  start_cv_.notify_all();

  if(main_thread_ && main_thread_->joinable())
  {
    main_thread_->join();
  }

  for(auto & t : threads_)
  {
    if(t.joinable())
    {
      t.join();
    }
  }

  mc_rtc::log::info("RobotManager shutdown complete.");
}

void RobotManager::init(const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("manager init start");

  mc_rtc::Configuration robots_config = gconfig_.config("Robots");
  mc_rtc::log::info(robots_config.dump(true, true));

  /* Set up robot interface and communication*/
  for(auto & robot_name : robots_config.keys())
  {
    mc_rtc::log::info("manager init robot {}", robot_name);

    if(interfaces_.count(robot_name) != 0)
    {
      mc_rtc::log::error("Skip already exists robot interface {}", robot_name);
      continue;
    }

    mc_rtc::Configuration robot_config{robots_config(robot_name)};
    std::unique_ptr<mc_robot::RobotInterfaceBase> interface =
        mc_robot::RobotInterfaceFactory::makeInterface(robot_name, robot_config);
    if(!interface)
    {
      continue;
    }

    interfaces_.try_emplace(robot_name, std::move(interface));
  }

  /* Send config to real robots */
  for(auto & [robot_name, interface] : interfaces_)
  {
    mc_rtc::log::info("manager init send config {}", robot_name);

    auto builder = mc_communication::Communication::serialize(interface->config());
    interface->communication().sendMessage(mc_communication::Communication::MessageType::CONFIG,
                                           builder.GetBufferPointer(), builder.GetSize());
  }

  /* Check timestep compatifibility between mc_rtc and robot */
  double controller_s = gcontroller_->controller().timeStep;
  size_t max_step_size{0};
  for(auto & [robot_name, interface] : interfaces_)
  {
    double cycle_s = interface->dt();
    auto cycle_ns = static_cast<size_t>(cycle_s * 1e9);
    auto controller_ns = static_cast<size_t>(controller_s * 1e9);
    if(controller_ns < cycle_ns)
    {
      mc_rtc::log::error_and_throw(
          "[mc_fleet] mc_rtc cannot run faster than the robot's control frequency (RobotTimeStep= {}s, Timestep={}s)",
          cycle_s, controller_s);
    }

    if(controller_ns % cycle_ns != 0)
    {
      mc_rtc::log::error_and_throw(
          "[mc_fleet] mc_rtc timestep must be a multiple of the robot's control loop frequency "
          "(RobotTimeStep= {}s, Timestep={}s)",
          cycle_s, controller_s);
    }

    size_t step_size = controller_ns / cycle_ns;
    size_t freq = std::ceil(1 / controller_s);
    size_t robot_freq = std::ceil(1 / cycle_s);
    mc_rtc::log::info("[mc_fleet] mc_rtc running at {}Hz, robot running at {}Hz", freq, robot_freq);

    if(max_step_size < step_size)
    {
      max_step_size = step_size;
    }
  }

  mc_rtc::log::info("[mc_fleet] mc_rtc will compute commands every {} robot control step", max_step_size);

  auto & robots = gcontroller_->controller().robots();
  /* Initialize all real robots */
  for(size_t i = gcontroller_->realRobots().size(); i < robots.size(); ++i)
  {
    gcontroller_->realRobots().robotCopy(robots.robot(i), robots.robot(i).name());
  }

  /* Init threads */
  gcontroller_->running = true;

  for(auto & [robot_name, interface] : interfaces_)
  {
    auto * interface_ptr = interface.get();
    threads_.emplace_back(
        [&, this, interface_ptr]()
        {
          interface_ptr->controlThread(*gcontroller_, start_mutex_, start_cv_, start_control_, gcontroller_->running);
        });
  }

  main_thread_ = std::make_unique<std::thread>(&RobotManager::mainThread, this, max_step_size, std::ref(interrupt));

  mc_rtc::log::info("manager init done");
}

void RobotManager::processGConfig(mc_control::MCGlobalController::GlobalConfiguration & gconfig)
{
  mc_rtc::log::success("manager processGConfig start");

  if(!gconfig.config.has("Robots"))
  {
    mc_rtc::log::error_and_throw<std::runtime_error>(
        "No `Robots` section in the configuration, see etc/mc_rtc.yaml for an example");
  }

  mc_rtc::log::info("manager processGConfig 1");

  if(gconfig.config.has("Default"))
  {
    mc_rtc::Configuration dc = gconfig.config("Default");
    user_default_.module = dc("module", std::string(user_default_.module));
    user_default_.control_mode = dc("control_mode", std::string(user_default_.control_mode));
    user_default_.driver = dc("driver", std::string(user_default_.driver));
    user_default_.time_step = dc("time_step", double(user_default_.time_step));
    user_default_.communication_protocol = dc("communication", std::string(user_default_.communication_protocol));
  }

  mc_rtc::log::info("manager processGConfig 2");

  mc_rtc::Configuration robots_config = gconfig.config("Robots");
  for(auto & robot_name : robots_config.keys())
  {
    mc_rtc::Configuration robot_config{gconfig.config("Robots")(robot_name)};

    if(robot_config.has("base"))
    {
      mc_rtc::Configuration base_config{};
      base_config.load(robots_config(robot_config("base")));
      base_config.load(robot_config);
      robot_config.load(base_config);
    }

    if(!robot_config.has("module"))
    {
      robot_config.add("module", user_default_.module);
    }

    if(!robot_config.has("controller"))
    {
      robot_config.add("controller");
      robot_config("controller").add("mode", user_default_.control_mode);
      robot_config("controller").add("driver", user_default_.driver);
      robot_config("controller").add("time_step", user_default_.time_step);
    }
    else
    {
      if(!robot_config("controller").has("mode"))
      {
        robot_config("controller").add("mode", user_default_.control_mode);
      }
      if(!robot_config("controller").has("driver"))
      {
        robot_config("controller").add("driver", user_default_.driver);
      }
      if(!robot_config("controller").has("time_step"))
      {
        robot_config("controller").add("time_step", user_default_.time_step);
      }
    }

    if(robot_config.has("communication"))
    {
      if(!robot_config("communication").has("protocol"))
      {
        robot_config("communication").add("protocol", user_default_.communication_protocol);
      }
    }
    else
    {
      mc_rtc::log::error_and_throw("No `communication` section in the configuration of robot {}", robot_name);
    }
  }

  mc_rtc::log::info("manager processGConfig done");
}

void RobotManager::mainThread(size_t step_size, const std::atomic<bool> & interrupt)
{

  std::mutex controller_mutex;
  size_t step = 0;

  while(gcontroller_->running)
  {
    // Synchronize with hardware
    std::unique_lock lock(controller_mutex);
    cv_.wait(lock);

    if(interrupt)
    {
      std::cout << "controller_run_ interrupted" << std::endl;
      gcontroller_->running = false;
      return;
    }

    for(auto & [robot_name, interface] : interfaces_)
    {
      interface->updateSensors();
    }

    if(step % step_size == 0)
    {
      gcontroller_->run();
      mc_rtc::log::info("main tick");
    }

    {
      std::lock_guard<std::mutex> lock(start_mutex_);
      start_control_ = true;
    }
    start_cv_.notify_all();

    for(auto & [robot_name, interface] : interfaces_)
    {
      interface->updateControl();
    }
    step++;
  }
}

} // namespace mc_fleet

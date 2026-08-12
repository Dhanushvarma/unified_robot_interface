#include <mc_robot_manager/Logging.h>
#include <mc_robot_manager/RobotManager.h>
#include <robot_comm/CommunicationZenoh.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <spawn.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

extern char ** environ;

namespace mc_fleet
{

namespace
{

// Directory containing the currently running MCFleetControl executable, so the
// co-located RobotInterface binary can be found without relying on PATH.
std::filesystem::path selfDir()
{
  std::error_code ec;
  auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
  if(ec) return {};
  return exe.parent_path();
}

} // namespace

RobotManager::RobotManager() : gconfig_(mc_rtc::Configuration{}) {};

RobotManager::RobotManager(const std::string & mc_config_path, const std::atomic<bool> & interrupt)
: gconfig_(mc_control::MCGlobalController::GlobalConfiguration(mc_config_path))
{
  processGConfig(gconfig_);
  // log::info(gconfig_.config("Robots").dump(true, true));

  gcontroller_ = std::make_unique<mc_control::MCGlobalController>(gconfig_);

  // // Connect to the signal
  // auto & mc_controller = gcontroller_->controller();
  // replace_slot_ = mc_controller.replaceRobot.connect(
  //     [&](const std::string & old_robot_name, const std::string & new_robot_name)
  //     {
  //       std::lock_guard<std::mutex> lock(replace_mutex_);
  //       replace_queue_.push({old_robot_name, new_robot_name});
  //       log::info("[mc_rtde] Signal caught: Request switching from {} to {}", old_robot_name,
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

  stopSpawnedInterfaces();

  log::info("RobotManager shutdown complete.");
}

bool RobotManager::autostartEnabled(const mc_rtc::Configuration & robot_config)
{
  return robot_config.has("robot_interface") && robot_config("robot_interface").has("autostart")
         && static_cast<bool>(robot_config("robot_interface")("autostart"));
}

pid_t RobotManager::spawnRobotInterface(const std::string & robot_name, const mc_rtc::Configuration & robot_config)
{
  // robot_interface only needs its own name plus the network/robot_interface sections.
  mc_rtc::Configuration iface_config;
  iface_config.add("name", robot_name);
  iface_config.add("network_interface", robot_config("network_interface"));
  iface_config.add("robot_interface", robot_config("robot_interface"));

  const auto config_path = std::filesystem::temp_directory_path() / ("mc_fleet_" + robot_name + "_interface.yaml");
  iface_config.save(config_path.string());

  auto bin_path = selfDir() / "RobotInterface";
  if(!std::filesystem::exists(bin_path))
  {
    bin_path = "RobotInterface"; // fall back to PATH lookup
  }

  log::info("[mc_fleet] Spawning co-located robot_interface for '", robot_name, "' (", bin_path.string(), ")");

  // posix_spawn (rather than fork()+exec()) avoids duplicating this process's
  // other threads (GUI server, ROS, ...) into the child: fork() in a
  // multi-threaded process only carries over the calling thread, so any lock
  // held by another thread at that instant is inherited pre-locked with no
  // owner left to release it. mlockall(MCL_FUTURE), active here since
  // main.cpp, makes plain fork() doubly unsafe (see fork(2)/mlockall(2)).
  const std::string bin_path_str = bin_path.string();
  const std::string config_path_str = config_path.string();
  std::array<char *, 6> argv{const_cast<char *>(bin_path_str.c_str()),    const_cast<char *>("-c"),
                             const_cast<char *>(config_path_str.c_str()), const_cast<char *>("-n"),
                             const_cast<char *>(robot_name.c_str()),      nullptr};

  pid_t pid = 0;
  int err = posix_spawnp(&pid, bin_path_str.c_str(), nullptr, nullptr, argv.data(), environ);
  if(err != 0)
  {
    log::error("[mc_fleet] posix_spawn failed while spawning robot_interface for '", robot_name,
               "': ", std::strerror(err));
    return -1;
  }

  spawned_interfaces_[robot_name] = pid;
  return pid;
}

void RobotManager::stopSpawnedInterfaces()
{
  for(auto & [robot_name, pid] : spawned_interfaces_)
  {
    if(kill(pid, SIGTERM) != 0) continue;

    int status = 0;
    for(int i = 0; i < 50; ++i) // wait up to ~5s for a clean exit
    {
      if(waitpid(pid, &status, WNOHANG) != 0) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if(waitpid(pid, &status, WNOHANG) == 0)
    {
      log::warning("[mc_fleet] robot_interface '", robot_name, "' (pid ", pid, ") did not exit, sending SIGKILL");
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
    }
  }
  spawned_interfaces_.clear();
}

void RobotManager::init(const std::atomic<bool> & interrupt)
{
  log::info("manager init start");

  mc_rtc::Configuration robots_config = gconfig_.config("Robots");

  /* Start Zenoh Router if necessary */
  if(robots_config.dump().find("zenoh") != std::string::npos)
  {
    launchZenohRouter();
  }

  /* Set up robot interface and communication*/
  for(auto & robot_name : robots_config.keys())
  {
    log::info("manager init robot ", robot_name);

    if(interfaces_.count(robot_name) != 0)
    {
      log::error("Skip already exists robot interface ", robot_name);
      continue;
    }

    mc_rtc::Configuration robot_config{robots_config(robot_name)};

    if(autostartEnabled(robot_config))
    {
      spawnRobotInterface(robot_name, robot_config);
    }

    std::unique_ptr<mc_robot::RobotInterfaceBase> interface =
        mc_robot::RobotInterfaceFactory::makeInterface(robot_name, robot_config);
    if(!interface)
    {
      continue;
    }

    interfaces_.try_emplace(robot_name, std::move(interface));
  }

  /* Send init query to each robot_interface process (query/reply: waits for driver load confirmation). */
  std::vector<std::string> failed_robots;
  for(auto & [robot_name, interface] : interfaces_)
  {
    log::info("[mc_fleet] Querying robot_interface '", robot_name, "' (waiting up to 10 s)…");

    const std::string init_topic = robot_name + "/init";
    auto config_payload = interface->communication().serializer()->serialize(interface->config().dump());

    auto reply = interface->communication().query(init_topic, config_payload, std::chrono::seconds(10));

    if(!reply)
    {
      log::error("[mc_fleet] Init query to robot_interface '", robot_name,
                 "' timed out — "
                 "is the process running? Skipping this robot.");
      failed_robots.push_back(robot_name);
      continue;
    }

    const std::string reply_str(reply->begin(), reply->end());
    if(reply_str != "OK")
    {
      log::error("[mc_fleet] Robot '", robot_name, "' init failed: ", reply_str, " — skipping.");
      failed_robots.push_back(robot_name);
      continue;
    }

    log::info("[mc_fleet] Robot '", robot_name, "' driver loaded successfully");
  }

  for(const auto & name : failed_robots)
  {
    interfaces_.erase(name);
  }

  if(interfaces_.empty())
  {
    log::errorAndThrow("[mc_fleet] No robot_interface responded — cannot start controller.");
  }

  /* Check timestep compatifibility between mc_rtc and robot */
  double controller_s = gcontroller_->controller().timeStep;
  size_t max_step_size{0};
  for(auto & [robot_name, interface] : interfaces_)
  {
    // double cycle_s = interface->dt();
    double cycle_s = 0.005;
    auto cycle_ns = static_cast<size_t>(cycle_s * 1e9);
    auto controller_ns = static_cast<size_t>(controller_s * 1e9);
    if(controller_ns < cycle_ns)
    {
      log::errorAndThrow("[mc_fleet] mc_rtc cannot run faster than the robot's control frequency (RobotTimeStep= ",
                         cycle_s, "s, Timestep=", controller_s, "s)");
    }

    if(controller_ns % cycle_ns != 0)
    {
      log::errorAndThrow("[mc_fleet] mc_rtc timestep must be a multiple of the robot's control loop frequency "
                         "(RobotTimeStep= ",
                         cycle_s, "s, Timestep=", controller_s, "s)");
    }

    size_t step_size = controller_ns / cycle_ns;
    size_t freq = std::ceil(1 / controller_s);
    size_t robot_freq = std::ceil(1 / cycle_s);
    log::info("[mc_fleet] mc_rtc running at ", freq, "Hz, robot running at ", robot_freq, "Hz");

    if(max_step_size < step_size)
    {
      max_step_size = step_size;
    }
  }

  log::info("[mc_fleet] mc_rtc will compute commands every ", max_step_size, "robot control step");

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

  log::info("manager init done");
}

void RobotManager::launchZenohRouter()
{
  if(zenoh_router_) return;

  log::info("[mc_fleet] launchZenohRouter start");

  zenoh::Config config =
      zenoh::Config::from_file("/home/vscode/workspace/sandbox/mc_rtc_interface/robot_comm/tests/zenoh/router.json5");

  zenoh_router_ = std::make_unique<zenoh::Session>(zenoh::Session::open(std::move(config)));

  // Let the router fully start before clients try to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  log::info("[mc_fleet] launchZenohRouter done");
}

void RobotManager::processGConfig(mc_control::MCGlobalController::GlobalConfiguration & gconfig)
{
  if(!gconfig.config.has("Robots"))
  {
    log::errorAndThrow("No `Robots` section in the configuration, see etc/mc_rtc.yaml for an example");
  }

  log::info("manager processGConfig 1");

  // Extract default value
  if(gconfig.config.has("Default"))
  {
    mc_rtc::Configuration dc = gconfig.config("Default");
    user_default_.module = dc("module", std::string(user_default_.module));
    user_default_.communication_protocol = dc("network_interface", std::string(user_default_.communication_protocol));
    user_default_.driver = dc("driver", std::string(user_default_.driver));
    user_default_.time_step = dc("time_step", double(user_default_.time_step));
    user_default_.control_mode = dc("control_mode", std::string(user_default_.control_mode));
  }

  log::info("manager processGConfig 2");

  mc_rtc::Configuration robots_config = gconfig.config("Robots");
  for(auto & robot_name : robots_config.keys())
  {
    mc_rtc::Configuration robot_config{gconfig.config("Robots")(robot_name)};

    // Copy config setting from a previous robot
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
      robot_config("controller").add("time_step", user_default_.time_step);
    }
    else
    {
      if(!robot_config("controller").has("mode"))
      {
        robot_config("controller").add("mode", user_default_.control_mode);
      }
      if(!robot_config("controller").has("time_step"))
      {
        robot_config("controller").add("time_step", user_default_.time_step);
      }
    }

    // robot_interface section carries the driver name and robot-side ip/port.
    // Fill in a default driver only if the whole section is absent.
    if(!robot_config.has("robot_interface"))
    {
      robot_config.add("robot_interface");
      robot_config("robot_interface").add("driver", user_default_.driver);
    }
    else if(!robot_config("robot_interface").has("driver"))
    {
      robot_config("robot_interface").add("driver", user_default_.driver);
    }

    if(robot_config.has("network_interface"))
    {
      if(!robot_config("network_interface").has("protocol"))
      {
        // A co-located robot_interface is guaranteed to share this host, so it can
        // use Zenoh's shared-memory transport by default instead of the network stack.
        const std::string default_protocol =
            autostartEnabled(robot_config) ? std::string{"zenoh/shm"} : user_default_.communication_protocol;
        robot_config("network_interface").add("protocol", default_protocol);
      }
    }
    else
    {
      log::errorAndThrow("No `network_interface` section in the configuration of robot ", robot_name);
    }
  }

  log::info("manager processGConfig done");
}

void RobotManager::mainThread(size_t step_size, const std::atomic<bool> & interrupt)
{
  size_t step = 0;

  double cycle_s = 0.005; // fallback: 200Hz
  if(!interfaces_.empty()) cycle_s = interfaces_.begin()->second->dt();

  using Clock = std::chrono::steady_clock;
  using Duration = std::chrono::duration<double>;
  auto next_wake = Clock::now();

  while(gcontroller_->running)
  {
    // Sleep until the next scheduled cycle tick.
    next_wake += std::chrono::duration_cast<Clock::duration>(Duration(cycle_s));
    std::this_thread::sleep_until(next_wake);

    if(interrupt)
    {
      gcontroller_->running = false;
      return;
    }

    for(auto & [robot_name, interface] : interfaces_) interface->updateSensors(*gcontroller_);

    // Gate controller.run() until every interface has fed its initial sensor
    // values and called gc.init(), otherwise the Posture task would command
    // the robot from a default (potentially far) configuration.
    bool all_ready =
        std::all_of(interfaces_.begin(), interfaces_.end(), [](const auto & kv) { return kv.second->isInitialized(); });

    if(all_ready && step % step_size == 0) gcontroller_->run();

    {
      std::lock_guard<std::mutex> lock(start_mutex_);
      start_control_ = true;
    }
    start_cv_.notify_all();

    for(auto & [robot_name, interface] : interfaces_) interface->updateControl(*gcontroller_);
    step++;
  }
}

} // namespace mc_fleet

#include <mc_rtc/logging.h>
#include <mc_robot_manager/RobotManager.h>

#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace mc_fleet
{

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
    user_default_.network_protocol = dc("network_protocol", std::string(user_default_.network_protocol));
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

    if(robot_config.has("network"))
    {
      if(!robot_config("network").has("protocol"))
      {
        robot_config("network").add("protocol", user_default_.network_protocol);
      }
    }
    else
    {
      mc_rtc::log::error_and_throw("No `network` section in the configuration of robot {}", robot_name);
    }
  }

  mc_rtc::log::info("manager processGConfig done");
}

void RobotManager::init()
{
  mc_rtc::log::success("manager init start");

  /* Check compatibility*/
  mc_rtc::Configuration robots_config = gconfig_.config("Robots");
  mc_rtc::log::info("manager init 1");
  for(auto & robot_name : robots_config.keys())
  {
    mc_rtc::Configuration robot_config{robots_config(robot_name)};
    robot_interface_factory_.addRobotInterface(robot_name, robot_config, network_interface_factory_);
  }

  // const char * homeDir = std::getenv("HOME");
  // std::string keyPath = std::string(homeDir) + "/workspace/sandbox/mc_robot_manager/CMakeLists.txt";
  // key_t key = ftok(keyPath.c_str(), 100);
  // // Get the Shared Memory Segment ID. Initialise the shared memory with 0600 permissions.
  // shmid_ = shmget(key, sizeof(mc_network::Message), 0666 | IPC_CREAT);
  // if(shmid_ == -1)
  // {
  //   mc_rtc::log::info("keyPath {}", keyPath);
  //   std::string error_msg = "shmget failed: " + std::string(strerror(errno));
  //   mc_rtc::log::error_and_throw("Error creating shared memory");
  // }
  // mc_rtc::log::success("Shared memory successfully created shmid {}", shmid_);

  mc_rtc::log::info("manager init done");
}

} // namespace mc_fleet

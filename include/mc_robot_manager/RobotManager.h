#pragma once

#include <mc_network_interface/NetworkInterface.h>
#include <mc_robot_interface/RobotInterfaceFactory.h>

#include <mc_control/mc_global_controller.h>

#include <thread>

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

void run(void * data, const std::atomic<bool> & interrupt);

void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt);

struct RobotManager
{

  // TODO: include gcontroller_ instead
  RobotManager(mc_control::MCGlobalController::GlobalConfiguration & gconfig)
  {
    processGConfig(gconfig);
    gcontroller_ = std::make_unique<mc_control::MCGlobalController>(gconfig);
    initNetworks();
    initRobots();
  };

  void processGConfig(mc_control::MCGlobalController::GlobalConfiguration & gconfig);

  void initNetworks();

  void initRobots();

private:
  DefaultConfig user_default_{"", "position", "", 0.001, "tcp"};

  std::unique_ptr<mc_control::MCGlobalController> gcontroller_;

  mc_robot::RobotInterfaceFactory robot_interface_factory_{};

  // TODO idea here is to have a thread for each network we are lauching for each robot
  // Would be nice to consider the case where robot are sharing the same timestep and protocol
  // Each thread is running at each robot dt.
  // Warning should be set in case the dt outreach protocol capacities
  std::unordered_map<std::thread::id, std::shared_ptr<mc_network_interface::NetworkInterface>> network_interfaces_;
};

void RobotManager::processGConfig(mc_control::MCGlobalController::GlobalConfiguration & gconfig)
{
  if(!gconfig.config.has("Robots"))
  {
    mc_rtc::log::error_and_throw<std::runtime_error>(
        "No `Robots` section in the configuration, see etc/mc_rtc.yaml for an example");
  }

  mc_rtc::log::info("mc_fleet::init 3");

  if(gconfig.config.has("Default"))
  {
    mc_control::Configuration dc = gconfig.config("Default");
    user_default_.module = dc("module", std::string(user_default_.module));
    user_default_.control_mode = dc("control_mode", std::string(user_default_.control_mode));
    user_default_.driver = dc("driver", std::string(user_default_.driver));
    user_default_.time_step = dc("time_step", double(user_default_.time_step));
    user_default_.network_protocol = dc("network_protocol", std::string(user_default_.network_protocol));
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
}

void RobotManager::initNetworks()
{
  mc_rtc::log::success("initNetworks 1");
}

void RobotManager::initRobots()
{
  mc_rtc::log::success("initRobots");

  mc_control::Configuration robots_config = gcontroller_->configuration().config("Robots");
  for(auto & robot_name : robots_config.keys())
  {
    mc_control::Configuration robot_config{robots_config(robot_name)};
    robot_interface_factory_.addRobotInterface(robot_name, robot_config);
  }

  mc_rtc::log::info("initRobots done");
}

} // namespace mc_fleet

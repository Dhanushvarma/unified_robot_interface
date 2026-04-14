#pragma once

#include <mc_network_interface/NetworkInterfaceFactory.h>
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

class RobotManager
{
public:
  RobotManager();
  // TODO: include gcontroller_ instead
  RobotManager(mc_control::MCGlobalController::GlobalConfiguration & gconfig) : gconfig_(gconfig)
  {
    processGConfig(gconfig_);
    // gcontroller_ = std::make_unique<mc_control::MCGlobalController>(gconfig);

    init();
  };

  void processGConfig(mc_control::MCGlobalController::GlobalConfiguration & gconfig);

  void init();

private:
  DefaultConfig user_default_{"", "position", "", 0.001, "tcp"};

  std::unique_ptr<mc_control::MCGlobalController> gcontroller_;
  mc_control::MCGlobalController::GlobalConfiguration gconfig_;

  mc_network::NetworkInterfaceFactory network_interface_factory_{};
  mc_robot::RobotInterfaceFactory robot_interface_factory_{};

  // mc_network::NetworkInterfaceSever network_interface_server_;

  // TODO idea here is to have a thread for each network we are lauching for each robot
  // Would be nice to consider the case where robot are sharing the same timestep and protocol
  // Each thread is running at each robot dt.
  // Warning should be set in case the dt outreach protocol capacities
};

} // namespace mc_fleet

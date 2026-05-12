#pragma once

#include <mc_communication/CommunicationFactory.h>
#include <mc_robot_interface/RobotInterfaceFactory.h>

#include <mc_control/mc_global_controller.h>

#include <atomic>
#include <string>
#include <thread>
#include <unordered_map>

namespace mc_fleet
{

void run(void * data, const std::atomic<bool> & interrupt);

void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt);

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
  struct DefaultConfig
  {
    std::string module{};
    std::string control_mode{"position"};
    std::string driver{};
    double time_step{0.001};
    std::string communication_protocol{"zenoh"};
  };

  DefaultConfig user_default_;

  std::unique_ptr<mc_control::MCGlobalController> gcontroller_;
  mc_control::MCGlobalController::GlobalConfiguration gconfig_;

  // mc_communication::CommunicationFactory communication_factory_{};
  // mc_robot::RobotInterfaceFactory interface_factory_{};

  std::unordered_map<std::string, std::unique_ptr<mc_robot::RobotInterface>> interfaces_{};

  // TODO idea here is to have a thread for each communication we are lauching for each robot
  // Would be nice to consider the case where robot are sharing the same timestep and protocol
  // Each thread is running at each robot dt.
  // Warning should be set in case the dt outreach protocol capacities
};

} // namespace mc_fleet

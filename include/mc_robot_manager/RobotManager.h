#pragma once

#include <thread>

#include <mc_control/mc_global_controller.h>
#include <mc_network_interface/NetworkInterface.h>

namespace mc_fleet
{

void run(void * data, const std::atomic<bool> & interrupt);

void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt);

struct RobotManager
{
  RobotManager();

private:
  std::unique_ptr<mc_control::MCGlobalController> gcontroller_;

  // TODO idea here is to have a thread for each network we are lauching for each robot
  // Would be nice to consider the case where robot are sharing the same timestep and protocol
  // Each thread is running at each robot dt.
  // Warning should be set in case the dt outreach protocol capacities
  std::unordered_map<std::thread, std::shared_ptr<mc_network_interface::NetworkInterface>> network_interfaces_;
};

} // namespace mc_fleet

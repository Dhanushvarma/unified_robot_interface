#pragma once

#include <thread>

#include <mc_control/mc_global_controller.h>
#include <mc_network_interface/NetworkInterface.h>

namespace mc_rtc {
struct RobotManager {
  RobotManager();

private:
  std::unique_ptr<mc_control::MCGlobalController> gc_;

  // TODO idea here is to have a thread for each network we are lauching for
  // each robot Would be nice to consider the case where robot are sharing the
  // same timestep and protocol Each thread is running at each robot dt. Warning
  // should be set in case the dt outreach protocol capacities
  std::unordered_map<std::thread,
                     std::shared_ptr<mc_network_interface::NetworkInterface>>
      network_interfaces_;
};
} // namespace mc_rtc

#pragma once

#include <atomic>
#include <string>

namespace mc_robot_interface
{

/// Entry point of `uri viewer`: loads the driver plugin named in the config's
/// robot_interface section directly (no manager, no communication stack) and
/// prints its live joint state at display_rate_hz until interrupt is set.
/// config_path is either a robot interface config or the manager's mc_rtc.yaml;
/// for the latter, name selects the robot under 'Robots' (optional when there
/// is only one). Returns the process exit code.
int runRobotStateViewer(const std::string & config_path,
                        const std::string & name,
                        double display_rate_hz,
                        const std::atomic<bool> & interrupt);

} // namespace mc_robot_interface

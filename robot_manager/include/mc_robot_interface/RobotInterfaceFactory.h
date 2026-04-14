#pragma once

#include <mc_rtc/Configuration.h>
#include <mc_network_interface/NetworkInterfaceFactory.h>
#include <mc_robot_interface/RobotInterface.h>
#include <string>

namespace mc_robot
{

struct RobotInterfaceFactory
{
public:
  /**
   * Constructs a new RobotInterface and stores it in
   * @ref robot_interfaces_ using the provided @p name as the key.
   *
   * Example of a config
   * @code .yaml
   * module: <robot_module>
   * init_pos: # [optional] default to all zeros
   *   translation: [x, y, z]
   *   rotation: [x, y, z]
   * controller:
   *   mode: <control_mode> # [optional] default to position
   *   driver: <driver>
   *   time_step: 0.001
   * network:
   *   protocol: tcp
   *   ip: 192.168.1.2
   *   port: 45000
   * @endcode
   *
   * @param name
   * @param config
   */
  void addRobotInterface(const std::string & name,
                         const mc_rtc::Configuration & config,
                         mc_network::NetworkInterfaceFactory & network_interface_factory);

  const std::unordered_map<std::string, std::unique_ptr<RobotInterface>> & robotInterfaces() const
  {
    return robot_interfaces_;
  }

private:
  std::unordered_map<std::string, std::unique_ptr<RobotInterface>> robot_interfaces_;
};

} // namespace mc_robot

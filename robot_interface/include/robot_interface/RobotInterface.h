#pragma once

#include <string>
#include <unordered_map>

#include <mc_communication/CommunicationFactory.h>

namespace mc_robot_interface
{
class RobotInterface
{
  RobotInterface(const std::string & name);

  inline const std::string & name() const
  {
    return name_;
  }

  void parseConfig(const mc_rtc::Configuration & config);

  template<typename... Args>
  void loadDriver(const std::string & name, const Args &... args);

  virtual void init();

  virtual void updateCommand();
  virtual void updateSensors();
  virtual void controlThread();

private:
  std::string name_;

protected:
  std::unique_ptr<mc_communication::Communication> communication_;
  std::unordered_map<std::string, std::shared_ptr<mc_communication::PublisherBase>> publishers_;

  std::unique_ptr<DriverBridge> driver_;
};
} // namespace mc_robot_interface

#pragma once

#include <robot_controller/Controller.h>

#include <mc_control/mc_global_controller.h>
namespace robot_controller
{

class ControllerMcRtc : public Controller
{
public:
  explicit ControllerMcRtc(const std::string & configFile);

  void run() override;
  bool isRunning() const override;
  void start() override;
  void stop() override;

  double timeStep() const override;

  void setEncoderValues(const std::string & robot, const std::vector<double> & values) override;
  void setEncoderVelocities(const std::string & robot, const std::vector<double> & values) override;
  void setJointTorques(const std::string & robot, const std::vector<double> & values) override;

  void setBodySensor(const std::string & robot,
                     const std::string & sensorName,
                     const std::array<double, 4> & orientation,
                     const std::array<double, 3> & angularVelocity,
                     const std::array<double, 3> & linearAcceleration) override;

  void setForceSensor(const std::string & robot,
                      const std::string & sensorName,
                      const std::array<double, 3> & force,
                      const std::array<double, 3> & torque) override;

  /// In the controller's datastore, as URI::<robot>::status (std::map<std::string, double>)
  void setStatus(const std::string & robot, const std::map<std::string, double> & values) override;

  /// From the controller's datastore entry URI::<robot>::command (std::map<std::string, double>), written by the
  /// controller or a plugin; none while there is no such entry
  std::map<std::string, double> commandValues(const std::string & robot) const override;

  void initializeRobots() override;

  void initializeRobot(const std::string & robotName, const std::vector<double> & encoderValues) override;

  std::vector<double> command(const std::string & robot_name, ControlMode mode) const override;

  mc_control::MCGlobalController & underlying()
  {
    return *impl_;
  }

private:
  std::unique_ptr<mc_control::MCGlobalController> impl_;
  /** Datastore key of each robot's status */
  std::map<std::string, std::string> statusKeys_;
};

} // namespace robot_controller

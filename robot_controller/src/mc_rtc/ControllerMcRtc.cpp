#include <robot_controller/mc_rtc/ControllerMcRtc.h>

#include <stdexcept>

namespace robot_controller
{

ControllerMcRtc::ControllerMcRtc(const std::string & configData)
{
  // This is quite troublesome. Can we update mc_control::MCGlobalController::GlobalConfiguration to accept
  // mc_rtc::Configuration directly ?
  auto tmp = std::filesystem::temp_directory_path() / "mc_rtc_processed.yaml";
  {
    std::ofstream out(tmp);
    out << configData;
  }

  impl_ = std::make_unique<mc_control::MCGlobalController>(
      mc_control::MCGlobalController::GlobalConfiguration(tmp.string()));

  std::filesystem::remove(tmp);
}

void ControllerMcRtc::run()
{
  impl_->run();
}

bool ControllerMcRtc::isRunning() const
{
  return impl_->running;
}

void ControllerMcRtc::start()
{
  impl_->running = true;
}

void ControllerMcRtc::stop()
{
  impl_->running = false;
}

double ControllerMcRtc::timeStep() const
{
  return impl_->controller().timeStep;
}

void ControllerMcRtc::setEncoderValues(const std::string & robot, const std::vector<double> & values)
{
  impl_->setEncoderValues(robot, values);
}

void ControllerMcRtc::setEncoderVelocities(const std::string & robot, const std::vector<double> & values)
{
  impl_->setEncoderVelocities(robot, values);
}

void ControllerMcRtc::setJointTorques(const std::string & robot, const std::vector<double> & values)
{
  impl_->setJointTorques(robot, values);
}

void ControllerMcRtc::setBodySensor(const std::string & robot,
                                    const std::string & sensorName,
                                    const std::array<double, 4> & orientation,
                                    const std::array<double, 3> & angularVelocity,
                                    const std::array<double, 3> & linearAcceleration)
{
  const Eigen::Quaterniond quat(orientation[0], orientation[1], orientation[2], orientation[3]);
  const Eigen::Vector3d angVel(angularVelocity[0], angularVelocity[1], angularVelocity[2]);
  const Eigen::Vector3d linAcc(linearAcceleration[0], linearAcceleration[1], linearAcceleration[2]);

  impl_->setSensorOrientations(robot, {{sensorName, quat}});
  impl_->setSensorAngularVelocities(robot, {{sensorName, angVel}});
  impl_->setSensorLinearAccelerations(robot, {{sensorName, linAcc}});
}

void ControllerMcRtc::setForceSensor(const std::string & robot,
                                     const std::string & sensorName,
                                     const std::array<double, 3> & force,
                                     const std::array<double, 3> & torque)
{
  // sva::ForceVecd takes (couple, force), in that order.
  const Eigen::Vector3d f(force[0], force[1], force[2]);
  const Eigen::Vector3d t(torque[0], torque[1], torque[2]);

  impl_->setWrenches(robot, {{sensorName, sva::ForceVecd(t, f)}});
}

void ControllerMcRtc::initializeRobot(const std::string & robotName, const std::vector<double> & encoderValues)
{
  auto & controller = impl_->controller();
  auto & robot = controller.robots().robot(robotName);

  const auto & jointOrder = robot.refJointOrder();

  if(encoderValues.size() != jointOrder.size())
  {
    throw std::invalid_argument("Cannot initialize robot '" + robotName + "': received "
                                + std::to_string(encoderValues.size()) + " encoder values but expected "
                                + std::to_string(jointOrder.size()));
  }

  // For MainRobot only
  const bool isMainRobot = controller.robot().name() == robot.name();
  if(isMainRobot)
  {
    impl_->init(encoderValues);
    return;
  }

  for(std::size_t i = 0; i < jointOrder.size(); ++i)
  {
    const auto jointIndex = robot.jointIndexInMBC(i);

    robot.mbc().q[jointIndex][0] = encoderValues[i];
  }

  robot.forwardKinematics();
  robot.forwardVelocity();

  auto & realRobot = controller.realRobots().robot(robotName);

  realRobot.mbc() = robot.mbc();
  realRobot.forwardKinematics();
  realRobot.forwardVelocity();
}

void ControllerMcRtc::initializeRobots()
{
  auto & robots = impl_->controller().robots();
  for(size_t i = impl_->realRobots().size(); i < robots.size(); ++i)
  {
    impl_->realRobots().robotCopy(robots.robot(i), robots.robot(i).name());
  }
}

std::vector<double> ControllerMcRtc::command(const std::string & robot_name, ControlMode mode) const
{
  const auto & robot = impl_->controller().robots().robot(robot_name);

  const std::size_t dof = robot.refJointOrder().size();
  std::vector<double> values(dof);

  switch(mode)
  {
    case ControlMode::POSITION:
    {
      for(size_t i = 0; i < dof; ++i) values[i] = robot.mbc().q[robot.jointIndexInMBC(i)][0];
      break;
    }

    case ControlMode::VELOCITY:
    {
      for(size_t i = 0; i < dof; ++i) values[i] = robot.mbc().alphaD[robot.jointIndexInMBC(i)][0];
      break;
    }

    case ControlMode::TORQUE:
    {
      for(size_t i = 0; i < dof; ++i) values[i] = robot.mbc().jointTorque[robot.jointIndexInMBC(i)][0];
      break;
    }
  }

  return values;
}

extern "C"
{

  const char * robot_controller_backend_name()
  {
    return "mc_rtc";
  }

  robot_controller::Controller * robot_controller_create(const char * config_data)
  {
    if(!config_data)
    {
      return nullptr;
    }

    try
    {
      return new robot_controller::ControllerMcRtc(std::string{config_data});
    }
    catch(...)
    {
      return nullptr;
    }
  }

  void robot_controller_destroy(robot_controller::Controller * controller)
  {
    delete controller;
  }
}

} // namespace robot_controller

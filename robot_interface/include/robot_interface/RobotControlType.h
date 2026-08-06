#pragma once

#include <mc_rbdyn/Robot.h>
#include <RBDyn/MultiBodyConfig.h>

#include <robot_interface/ControlMode.h>

namespace mc_robot_interface
{

template<ControlMode cm>
struct RobotControlType
{
  static_assert(static_cast<int>(cm) == static_cast<int>(cm) + 1, "This must be specialized");

  void init(unsigned int dof)
  {
    command_ = std::vector<double>(dof, 0.0);
  }

protected
  ::std::vector<double> command_;
};

template<>
struct RobotControlType<ControlMode::Position>
{
private:
  std::vector<double> q = std::vector<double>(6, 0.0);

public:
  void control(DriverBridge & driverBridge, const mc_rbdyn::Robot & robot, const rbd::MultiBodyConfig & mbc)
  {
    const auto & rjo = robot.refJointOrder();
    for(size_t i = 0; i < rjo.size(); ++i)
    {
      q[i] = mbc.q[robot.jointIndexInMBC(i)][0];
    }

    driverBridge.servoJ(q);
  }
};

template<>
struct RobotControlType<ControlMode::Velocity>
{
private:
  std::vector<double> dq = std::vector<double>(6, 0.0);

public:
  void control(DriverBridge & driverBridge, const mc_rbdyn::Robot & robot, const rbd::MultiBodyConfig & mbc)
  {
    const auto & rjo = robot.refJointOrder();
    for(size_t i = 0; i < dq.size(); ++i)
    {
      dq[i] = mbc.alphaD[robot.jointIndexByName(rjo[i])][0];
    }

    driverBridge.speedJ(dq);
  }
};

template<>
struct RobotControlType<ControlMode::Torque>
{
private:
  double acceleration = 0.5;
  double dt = 0.002;

public:
  void control(DriverBridge & driverBridge, const mc_rbdyn::Robot & robot, const rbd::MultiBodyConfig & mbc)
  {
    // auto start_t = robot_.initPeriod();
    // // TODO check the documentation
    // robot_.waitPeriod(start_t);
  }
};

class RobotControllerTemplate
{
};
} // namespace mc_robot_interface

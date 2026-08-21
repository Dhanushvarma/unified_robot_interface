#include <triorb_driver/RobotDriverTriOrb.h>

#include <fmt/core.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <thread>
#include <utility>

namespace triorb_driver
{

RobotDriverTriOrb::RobotDriverTriOrb(const std::string & ip, uint16_t port)
: client_(std::make_unique<TriOrbClient>(makeHttpTransport(ip, port == 0 ? defaultHttpPort_ : port, requestTimeout_)))
{
  fmt::print("[RobotDriverTriOrb] Connecting to {}:{}\n", ip, port == 0 ? defaultHttpPort_ : port);

  try
  {
    connect();
  }
  catch(...)
  {
    disconnect();
    throw;
  }

  fmt::print("[RobotDriverTriOrb] Connected\n");
}

RobotDriverTriOrb::RobotDriverTriOrb(std::unique_ptr<ITriOrbClient> client,
                                     bool wakeupOnConnect,
                                     bool sleepOnDisconnect)
: client_(std::move(client)), wakeupOnConnect_(wakeupOnConnect), sleepOnDisconnect_(sleepOnDisconnect)
{
  if(!client_)
  {
    throw std::invalid_argument("RobotDriverTriOrb requires a client");
  }
}

RobotDriverTriOrb::~RobotDriverTriOrb()
{
  disconnect();
}

void RobotDriverTriOrb::connect()
{
  client_->checkHealth();

  if(wakeupOnConnect_)
  {
    client_->wakeup();

    std::this_thread::sleep_for(wakeupSettle_);
  }

  command_ = {};

  // Start from a zero body-frame velocity.
  client_->setBodyVelocity(0.0, 0.0, 0.0);

  pose_ = client_->readPose();
  connected_ = true;
}

void RobotDriverTriOrb::disconnect() noexcept
{
  if(!client_ || !connected_)
  {
    return;
  }

  connected_ = false;
  command_ = {};

  try
  {
    client_->setBodyVelocity(0.0, 0.0, 0.0);
  }
  catch(const std::exception & error)
  {
    fmt::print(stderr, "[RobotDriverTriOrb] zero-velocity command failed: {}\n", error.what());
  }

  try
  {
    client_->stop();
  }
  catch(const std::exception & error)
  {
    fmt::print(stderr, "[RobotDriverTriOrb] stop failed: {}\n", error.what());
  }

  if(sleepOnDisconnect_)
  {
    try
    {
      client_->sleep();
    }
    catch(const std::exception & error)
    {
      fmt::print(stderr, "[RobotDriverTriOrb] motor sleep failed: {}\n", error.what());
    }
  }
}

void RobotDriverTriOrb::paceCycle()
{
  if(!timerInitialized_)
  {
    nextCycle_ = Clock::now();
    timerInitialized_ = true;
  }

  nextCycle_ += cyclePeriod_;

  const auto now = Clock::now();

  if(nextCycle_ > now)
  {
    std::this_thread::sleep_until(nextCycle_);
  }
  else
  {
    nextCycle_ = now;
  }
}

void RobotDriverTriOrb::sync()
{
  if(!connected_)
  {
    throw std::runtime_error("RobotDriverTriOrb is not connected");
  }

  try
  {
    client_->setBodyVelocity(command_.vx, command_.vy, command_.wz);

    const auto latestPose = client_->readPose();

    if(latestPose.valid)
    {
      pose_ = latestPose;
    }
  }
  catch(...)
  {
    command_ = {};

    try
    {
      client_->stop();
    }
    catch(...)
    {
    }

    connected_ = false;
    throw;
  }

  paceCycle();
}

std::vector<double> RobotDriverTriOrb::getActualQ()
{
  if(!pose_.valid)
  {
    return {};
  }

  return {pose_.x, pose_.y, pose_.theta};
}

std::vector<double> RobotDriverTriOrb::getActualQd()
{
  return {};
}

std::vector<double> RobotDriverTriOrb::getJointTorques()
{
  return {};
}

void RobotDriverTriOrb::validateVelocity(const std::vector<double> & velocity) const
{
  if(velocity.size() != 3)
  {
    throw std::invalid_argument("RobotDriverTriOrb velocity command must contain "
                                "exactly [vx, vy, wz]");
  }

  if(!std::all_of(velocity.begin(), velocity.end(), [](double value) { return std::isfinite(value); }))
  {
    throw std::invalid_argument("RobotDriverTriOrb velocity command contains "
                                "a non-finite value");
  }
}

void RobotDriverTriOrb::speedJ(const std::vector<double> & velocity)
{
  validateVelocity(velocity);

  command_.vx = velocity[0];
  command_.vy = velocity[1];
  command_.wz = velocity[2];
}

void RobotDriverTriOrb::servoJ(const std::vector<double> &)
{
  throw std::logic_error("RobotDriverTriOrb does not support servoJ");
}

void RobotDriverTriOrb::tauJ(const std::vector<double> &)
{
  throw std::logic_error("RobotDriverTriOrb does not support tauJ");
}

bool RobotDriverTriOrb::freeDrive(bool enable)
{
  if(enable)
  {
    client_->setBodyVelocity(0.0, 0.0, 0.0);

    client_->sleep();
  }
  else
  {
    client_->wakeup();

    std::this_thread::sleep_for(wakeupSettle_);
  }

  return true;
}

} // namespace triorb_driver

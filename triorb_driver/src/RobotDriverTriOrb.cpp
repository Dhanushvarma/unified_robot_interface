#include <triorb_driver/RobotDriverTriOrb.h>

#include <fmt/core.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <utility>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace
{

// TriOrb RobotCodes.
// Values are transmitted as little-endian uint16_t.
constexpr uint16_t CODE_GET_POSE = 0x010D;

constexpr uint16_t CODE_ORIGIN_RESET = 0x0203;

constexpr uint16_t CODE_STARTUP_SUSPENSION = 0x0301;

constexpr uint16_t CODE_MOVING_SPEED_RELATIVE = 0x030F;

constexpr uint16_t CODE_MOVING_DRIVE_LIFE_TIME = 0x0323;

// TriOrb RobotValues.
constexpr uint8_t VALUE_ROBOT_SUSPENSION = 0x01;

constexpr uint8_t VALUE_ROBOT_STARTUP = 0x02;

// Serial frame markers.
constexpr uint8_t FRAME_START = 0x00;

constexpr uint8_t FRAME_CR = 0x0D;

constexpr uint8_t FRAME_LF = 0x0A;

// x, y, theta are three 32-bit floating-point values.
constexpr std::size_t POSE_PAYLOAD_SIZE = 12;

// constexpr double PI = 3.14159265358979323846;

speed_t toTermiosSpeed(unsigned int baudrate)
{
  switch(baudrate)
  {
    case 9600:
      return B9600;

    case 19200:
      return B19200;

    case 38400:
      return B38400;

    case 57600:
      return B57600;

    case 115200:
      return B115200;

#ifdef B230400
    case 230400:
      return B230400;
#endif

#ifdef B460800
    case 460800:
      return B460800;
#endif

#ifdef B921600
    case 921600:
      return B921600;
#endif

    default:
      throw std::invalid_argument("Unsupported TriOrb baud rate: " + std::to_string(baudrate));
  }
}

double monotonicNow()
{
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void appendU16LittleEndian(std::vector<uint8_t> & output, uint16_t value)
{
  output.push_back(static_cast<uint8_t>(value & 0xFF));

  output.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void appendU32LittleEndian(std::vector<uint8_t> & output, uint32_t value)
{
  output.push_back(static_cast<uint8_t>(value & 0xFF));

  output.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));

  output.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));

  output.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void appendFloat32LittleEndian(std::vector<uint8_t> & output, float value)
{
  static_assert(sizeof(float) == sizeof(uint32_t), "TriOrb protocol requires 32-bit float");

  uint32_t bits = 0;

  std::memcpy(&bits, &value, sizeof(bits));

  appendU32LittleEndian(output, bits);
}

float unpackFloat32LittleEndian(const uint8_t * data)
{
  const uint32_t bits = static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8)
                        | (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);

  float value = 0.0F;

  std::memcpy(&value, &bits, sizeof(value));

  return value;
}

std::vector<uint8_t> makePlanarPayload(double x, double y, double theta)
{
  std::vector<uint8_t> payload;
  payload.reserve(POSE_PAYLOAD_SIZE);

  appendFloat32LittleEndian(payload, static_cast<float>(x));

  appendFloat32LittleEndian(payload, static_cast<float>(y));

  appendFloat32LittleEndian(payload, static_cast<float>(theta));

  return payload;
}

long findValueAfterCode(const std::vector<uint8_t> & buffer, uint16_t code, std::size_t startOffset)
{
  if(buffer.size() < 2)
  {
    return -1;
  }

  const uint8_t low = static_cast<uint8_t>(code & 0xFF);

  const uint8_t high = static_cast<uint8_t>((code >> 8) & 0xFF);

  for(std::size_t i = startOffset; i + 1 < buffer.size(); ++i)
  {
    if(buffer[i] == low && buffer[i + 1] == high)
    {
      return static_cast<long>(i + 2);
    }
  }

  return -1;
}

} // namespace

namespace triorb_driver
{

RobotDriverTriOrb::RobotDriverTriOrb(const std::string & device, uint16_t) : device_("/dev/ttyACM0")
{
  // TODO: test
  positionControlConfig_.translationGain = 1.0;
  positionControlConfig_.rotationGain = 1.0;

  positionControlConfig_.maximumLinearVelocity = 0.1;
  positionControlConfig_.maximumAngularVelocity = 0.2;

  positionControlConfig_.positionTolerance = 0.005;
  positionControlConfig_.angularTolerance = 0.01;
  //

  if(device_.empty())
  {
    throw std::invalid_argument("TriOrb serial device must not be empty");
  }

  fmt::print("[RobotDriverTriOrb] Connecting to {} at {} baud\n", device_, defaultBaudrate_);

  try
  {
    connect();
  }
  catch(...)
  {
    disconnect();
    throw;
  }

  fmt::print("[RobotDriverTriOrb] Connected to {}\n", device_);
}

RobotDriverTriOrb::~RobotDriverTriOrb()
{
  disconnect();
}

void RobotDriverTriOrb::connect()
{
  if(!openPort())
  {
    throw std::runtime_error("Failed to open TriOrb serial device " + device_ + ": " + std::strerror(errno));
  }

  if(!sendWakeup())
  {
    throw std::runtime_error("TriOrb motor wakeup command failed");
  }

  std::this_thread::sleep_for(wakeupSettle_);

  command_ = {};

  if(!sendStop())
  {
    throw std::runtime_error("TriOrb initial zero-velocity command failed");
  }

  if(!sendResetOrigin())
  {
    throw std::runtime_error("TriOrb origin reset command failed");
  }

  if(!readOdometry())
  {
    throw std::runtime_error("TriOrb initial odometry request failed");
  }

  targetPose_ = pose_;
  targetPose_.valid = true;

  connected_ = true;
}

void RobotDriverTriOrb::disconnect() noexcept
{
  if(fd_ < 0)
  {
    connected_ = false;
    return;
  }

  connected_ = false;
  command_ = {};

  try
  {
    if(!sendStop())
    {
      fmt::print(stderr, "[RobotDriverTriOrb] stop command failed during disconnect\n");
    }
  }
  catch(const std::exception & error)
  {
    fmt::print(stderr, "[RobotDriverTriOrb] stop exception during disconnect: {}\n", error.what());
  }
  catch(...)
  {
    fmt::print(stderr, "[RobotDriverTriOrb] unknown stop exception during disconnect\n");
  }

  try
  {
    if(!sendSleep())
    {
      fmt::print(stderr, "[RobotDriverTriOrb] motor sleep command failed during disconnect\n");
    }
  }
  catch(const std::exception & error)
  {
    fmt::print(stderr, "[RobotDriverTriOrb] motor sleep exception: {}\n", error.what());
  }
  catch(...)
  {
    fmt::print(stderr, "[RobotDriverTriOrb] unknown motor sleep exception\n");
  }

  closePort();

  fmt::print("[RobotDriverTriOrb] Disconnected from {}\n", device_);
}

bool RobotDriverTriOrb::openPort()
{
  closePort();

  fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

  if(fd_ < 0)
  {
    return false;
  }

  const int currentFlags = ::fcntl(fd_, F_GETFL, 0);

  if(currentFlags < 0)
  {
    closePort();
    return false;
  }

  if(::fcntl(fd_, F_SETFL, currentFlags & ~O_NONBLOCK) < 0)
  {
    closePort();
    return false;
  }

  termios tty{};

  if(::tcgetattr(fd_, &tty) != 0)
  {
    closePort();
    return false;
  }

  const speed_t speed = toTermiosSpeed(defaultBaudrate_);

  if(::cfsetispeed(&tty, speed) != 0 || ::cfsetospeed(&tty, speed) != 0)
  {
    closePort();
    return false;
  }

  // 8 data bits, no parity, one stop bit.
  tty.c_cflag &= ~static_cast<tcflag_t>(PARENB);

  tty.c_cflag &= ~static_cast<tcflag_t>(CSTOPB);

  tty.c_cflag &= ~static_cast<tcflag_t>(CSIZE);

  tty.c_cflag |= CS8;

#ifdef CRTSCTS
  tty.c_cflag &= ~static_cast<tcflag_t>(CRTSCTS);
#endif

  tty.c_cflag |= static_cast<tcflag_t>(CLOCAL | CREAD);

  // Raw input.
  tty.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO | ECHOE | ISIG);

  tty.c_iflag &= ~static_cast<tcflag_t>(IXON | IXOFF | IXANY);

  tty.c_iflag &= ~static_cast<tcflag_t>(ICRNL | INLCR | IGNCR | ISTRIP | INPCK | PARMRK | BRKINT);

  // Raw output.
  tty.c_oflag &= ~static_cast<tcflag_t>(OPOST | ONLCR);

  const int timeoutDeciseconds = std::max(1, static_cast<int>(readTimeout_ * 10.0 + 0.5));

  tty.c_cc[VMIN] = 0;

  tty.c_cc[VTIME] = static_cast<cc_t>(timeoutDeciseconds);

  if(::tcsetattr(fd_, TCSANOW, &tty) != 0)
  {
    closePort();
    return false;
  }

  flushIo();

  return true;
}

void RobotDriverTriOrb::closePort() noexcept
{
  if(fd_ >= 0)
  {
    ::close(fd_);
    fd_ = -1;
  }
}

void RobotDriverTriOrb::flushIo()
{
  if(fd_ >= 0)
  {
    ::tcflush(fd_, TCIOFLUSH);
  }
}

bool RobotDriverTriOrb::writeAll(const uint8_t * data, std::size_t size)
{
  if(fd_ < 0)
  {
    return false;
  }

  std::size_t written = 0;

  while(written < size)
  {
    const ssize_t result = ::write(fd_, data + written, size - written);

    if(result < 0)
    {
      if(errno == EINTR)
      {
        continue;
      }

      return false;
    }

    if(result == 0)
    {
      return false;
    }

    written += static_cast<std::size_t>(result);
  }

  return true;
}

bool RobotDriverTriOrb::readFrame(std::size_t expectedLength, std::vector<uint8_t> & response)
{
  response.clear();

  if(fd_ < 0)
  {
    return false;
  }

  const double deadline = monotonicNow() + frameTimeout_;

  uint8_t chunk[64];

  while(monotonicNow() < deadline)
  {
    const ssize_t received = ::read(fd_, chunk, sizeof(chunk));

    if(received < 0)
    {
      if(errno == EINTR)
      {
        continue;
      }

      return false;
    }

    if(received == 0)
    {
      continue;
    }

    response.insert(response.end(), chunk, chunk + received);

    const bool endsWithCrLf =
        response.size() >= 3 && response[response.size() - 2] == FRAME_CR && response[response.size() - 1] == FRAME_LF;

    if(endsWithCrLf && (response.size() >= expectedLength || response.size() == 3))
    {
      return true;
    }
  }

  return false;
}

std::vector<uint8_t> RobotDriverTriOrb::buildFrame(const std::vector<Command> & commands) const
{
  std::vector<uint8_t> frame;

  frame.push_back(FRAME_START);

  for(const auto & command : commands)
  {
    appendU16LittleEndian(frame, command.code);

    frame.insert(frame.end(), command.payload.begin(), command.payload.end());
  }

  frame.push_back(FRAME_CR);

  frame.push_back(FRAME_LF);

  return frame;
}

bool RobotDriverTriOrb::transact(const std::vector<Command> & commands, std::vector<uint8_t> & response)
{
  transactionOk_ = false;

  if(fd_ < 0)
  {
    return false;
  }

  std::size_t expectedLength = 1 + 2;

  for(const auto & command : commands)
  {
    expectedLength += 2 + command.payload.size();
  }

  const auto frame = buildFrame(commands);

  if(!writeAll(frame.data(), frame.size()))
  {
    flushIo();
    return false;
  }

  if(!readFrame(expectedLength, response))
  {
    flushIo();
    return false;
  }

  // START + CR + LF is the rejection response.
  if(response.size() <= 3)
  {
    return false;
  }

  transactionOk_ = true;

  return true;
}

bool RobotDriverTriOrb::sendWakeup()
{
  std::vector<uint8_t> response;

  return transact(
      {
          {
              CODE_STARTUP_SUSPENSION,
              {VALUE_ROBOT_STARTUP},
          },
      },
      response);
}

bool RobotDriverTriOrb::sendSleep()
{
  std::vector<uint8_t> response;

  return transact(
      {
          {
              CODE_STARTUP_SUSPENSION,
              {VALUE_ROBOT_SUSPENSION},
          },
      },
      response);
}

bool RobotDriverTriOrb::sendResetOrigin()
{
  std::vector<uint8_t> response;

  return transact(
      {
          {
              CODE_ORIGIN_RESET,
              {0x01},
          },
      },
      response);
}

bool RobotDriverTriOrb::sendVelocity(double vx, double vy, double wz)
{
  std::vector<Command> commands;

  commands.push_back({
      CODE_MOVING_SPEED_RELATIVE,
      makePlanarPayload(vx, vy, wz),
  });

  std::vector<uint8_t> lifetime;

  appendU32LittleEndian(lifetime, watchdogMs_);

  commands.push_back({
      CODE_MOVING_DRIVE_LIFE_TIME,
      std::move(lifetime),
  });

  std::vector<uint8_t> response;

  return transact(commands, response);
}

bool RobotDriverTriOrb::sendStop()
{
  std::vector<uint8_t> response;

  return transact(
      {
          {
              CODE_MOVING_SPEED_RELATIVE,
              makePlanarPayload(0.0, 0.0, 0.0),
          },
      },
      response);
}

bool RobotDriverTriOrb::readOdometry()
{
  std::vector<uint8_t> response;

  const bool received = transact(
      {
          {
              CODE_GET_POSE,
              std::vector<uint8_t>(POSE_PAYLOAD_SIZE, 0),
          },
      },
      response);

  if(!received)
  {
    return false;
  }

  const long valueOffset = findValueAfterCode(response, CODE_GET_POSE, 1);

  if(valueOffset < 0)
  {
    transactionOk_ = false;
    return false;
  }

  const auto offset = static_cast<std::size_t>(valueOffset);

  if(offset + POSE_PAYLOAD_SIZE > response.size())
  {
    transactionOk_ = false;
    return false;
  }

  const uint8_t * data = response.data() + offset;

  const double x = unpackFloat32LittleEndian(data);

  const double y = unpackFloat32LittleEndian(data + 4);

  const double thetaRadians = unpackFloat32LittleEndian(data + 8);

  if(!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(thetaRadians))
  {
    transactionOk_ = false;
    return false;
  }

  pose_.x = x;
  pose_.y = y;
  pose_.theta = thetaRadians;
  pose_.valid = true;

  return true;
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

  if(!sendVelocity(command_.vx, command_.vy, command_.wz))
  {
    command_ = {};

    try
    {
      sendStop();
    }
    catch(...)
    {
    }

    connected_ = false;

    throw std::runtime_error("TriOrb velocity transaction failed");
  }

  if(!readOdometry())
  {
    command_ = {};

    try
    {
      sendStop();
    }
    catch(...)
    {
    }

    connected_ = false;

    throw std::runtime_error("TriOrb odometry transaction failed");
  }

  if(controlMode_ == ControlMode::Position)
  {
    const auto velocity = computePlanarVelocity(pose_, targetPose_, positionControlConfig_);

    command_.vx = velocity.vx;
    command_.vy = velocity.vy;
    command_.wz = velocity.wz;

    static std::size_t count = 0;

    if(count++ % 200 == 0)
    {
      fmt::print(stderr,
                 "[RobotDriverTriOrb] position control: "
                 "measured=[{}, {}, {}], "
                 "target=[{}, {}, {}], "
                 "velocity=[{}, {}, {}]\n",
                 pose_.x, pose_.y, pose_.theta, targetPose_.x, targetPose_.y, targetPose_.theta, command_.vx,
                 command_.vy, command_.wz);
    }
  }

  // TODO: delete debug log
  static std::size_t count = 0;

  if(count++ % 200 == 0)
  {
    fmt::print(stderr,
               "[RobotDriverTriOrb] odometry: "
               "x={} m, y={} m, theta={} rad\n",
               pose_.x, pose_.y, pose_.theta);
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

  // TODO: delete debug log
  static std::size_t count = 0;

  if(count++ % 200 == 0)
  {
    fmt::print(stderr,
               "[RobotDriverTriOrb] speedJ: "
               "vx={} m/s, vy={} m/s, wz={} rad/s\n",
               velocity[0], velocity[1], velocity[2]);
  }
  //

  command_.vx = velocity[0];
  command_.vy = velocity[1];
  command_.wz = velocity[2];

  controlMode_ = ControlMode::Velocity;
}

void RobotDriverTriOrb::servoJ(const std::vector<double> & position)
{
  if(position.size() != 3)
  {
    throw std::invalid_argument("RobotDriverTriOrb position command must contain exactly "
                                "[x, y, theta]");
  }

  if(!std::all_of(position.begin(), position.end(), [](double value) { return std::isfinite(value); }))
  {
    throw std::invalid_argument("RobotDriverTriOrb position command contains a non-finite value");
  }

  static std::size_t count = 0;

  if(count++ % 200 == 0)
  {
    fmt::print(stderr,
               "[RobotDriverTriOrb] position target: "
               "x={} m, y={} m, theta={} rad\n",
               position[0], position[1], position[2]);
  }

  targetPose_.x = position[0];
  targetPose_.y = position[1];
  targetPose_.theta = position[2];
  targetPose_.valid = true;

  controlMode_ = ControlMode::Position;
}

void RobotDriverTriOrb::tauJ(const std::vector<double> &)
{
  throw std::logic_error("RobotDriverTriOrb does not support tauJ");
}

} // namespace triorb_driver

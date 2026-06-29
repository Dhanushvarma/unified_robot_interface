#pragma once

#include <mc_control/mc_global_controller.h>
#include <mc_rtc/Configuration.h>

#include <mc_communication/Communication.h>
#include <mc_communication/CommunicationFactory.h>
#include <mc_robot_interface/RobotDriver.h>

#include <condition_variable>

namespace mc_robot
{

class RobotInterfaceBase
{
public:
  RobotInterfaceBase() = default;

  RobotInterfaceBase(std::string name, mc_rtc::Configuration config, uint8_t buffer_size)
  : name_(std::move(name)), config_(std::move(config)), dt_(config_("controller")("time_step")),
    buffer_size_(buffer_size) {};

  virtual ~RobotInterfaceBase() = default;
  RobotInterfaceBase(const RobotInterfaceBase &) = delete;
  RobotInterfaceBase & operator=(const RobotInterfaceBase &) = delete;
  RobotInterfaceBase(RobotInterfaceBase &&) = delete;
  RobotInterfaceBase & operator=(RobotInterfaceBase &&) = delete;

  virtual void init() = 0;
  virtual void reset() = 0;
  virtual void stop() = 0;

  virtual void updateSensors() = 0;
  virtual void updateControl() = 0;

  /**
   * @brief Method in charge of robot sensors and commands update
   *
   */
  void controlThread(mc_control::MCGlobalController & controller,
                     std::mutex & startM,
                     std::condition_variable & start_cv_,
                     bool & start,
                     bool & running) {};

  // TODO: delete logging
  void dumpLog(const std::string & filename) const;

  template<typename cm>
  void control();

  void setCommunication(std::unique_ptr<mc_communication::Communication> communication)
  {
    communication_ = std::move(communication);
  }

  [[nodiscard]] const std::string & name() const
  {
    return name_;
  }
  [[nodiscard]] mc_rtc::Configuration & config()
  {
    return config_;
  }
  [[nodiscard]] double dt() const
  {
    return dt_;
  }
  [[nodiscard]] uint8_t bufferSize() const
  {
    return buffer_size_;
  }

  [[nodiscard]] mc_communication::State & state()
  {
    return state_;
  }

  [[nodiscard]] mc_communication::Command & command()
  {
    return command_;
  }

  // TODO: delete logging
  [[nodiscard]] std::vector<mc_communication::State> & states()
  {
    return states_;
  }

  [[nodiscard]] std::vector<mc_communication::Command> & commands()
  {
    return commands_;
  }

  [[nodiscard]] mc_communication::Communication & communication()
  {
    return *communication_;
  }

  void setConfig(const mc_rtc::Configuration & config)
  {
    config_ = config;
  }

  void setState(const mc_communication::State & state)
  {
    state_ = state;
    states_.push_back(state);
  }

  void setCommand(const mc_communication::Command & command)
  {
    command_ = command;
    commands_.push_back(command);
  }

private:
  mutable std::mutex update_sensor_mtx_{};
  mutable std::mutex update_control_mtx_{};

  const std::string name_{};
  mc_rtc::Configuration config_{};
  const double dt_{};

  std::unique_ptr<mc_communication::Communication> communication_{};

  mutable std::mutex mutex_;
  mc_communication::State state_{};
  mc_communication::Command command_{};

  // TODO: delete logging
  std::vector<mc_communication::State> states_;
  std::vector<mc_communication::Command> commands_;

  // TODO: control mode ?
};

// TODO: delete logging
inline void RobotInterfaceBase::dumpLog(const std::string & filename) const
{
  std::ofstream file(filename);
  if(!file.is_open())
  {
    mc_rtc::log::error("Failed to open log file: {}", filename);
    return;
  }

  file << "{\n";

  // ── States ──
  file << "  \"states\": [\n";
  for(size_t i = 0; i < states_.size(); ++i)
  {
    const auto & s = states_[i];
    file << "    {\n";
    file << "      \"index\": " << i << ",\n";

    file << "      \"position\": [";
    for(size_t j = 0; j < s.position.size(); ++j)
    {
      file << s.position[j];
      if(j + 1 < s.position.size())
      {
        file << ", ";
      }
    }
    file << "],\n";

    file << "      \"velocity\": [";
    for(size_t j = 0; j < s.velocity.size(); ++j)
    {
      file << s.velocity[j];
      if(j + 1 < s.velocity.size())
      {
        file << ", ";
      }
    }
    file << "],\n";

    file << "      \"torque\": [";
    for(size_t j = 0; j < s.torque.size(); ++j)
    {
      file << s.torque[j];
      if(j + 1 < s.torque.size())
      {
        file << ", ";
      }
    }
    file << "]\n";

    file << "    }";
    if(i + 1 < states_.size())
    {
      file << ",";
    }
    file << "\n";
  }
  file << "  ],\n";

  // ── Commands ──
  file << "  \"commands\": [\n";
  for(size_t i = 0; i < commands_.size(); ++i)
  {
    const auto & c = commands_[i];
    file << "    {\n";
    file << "      \"index\": " << i << ",\n";
    file << "      \"kp\": " << c.kp << ",\n";
    file << "      \"kd\": " << c.kd << ",\n";

    file << "      \"position\": [";
    for(size_t j = 0; j < c.position.size(); ++j)
    {
      file << c.position[j];
      if(j + 1 < c.position.size())
      {
        file << ", ";
      }
    }
    file << "],\n";

    file << "      \"velocity\": [";
    for(size_t j = 0; j < c.velocity.size(); ++j)
    {
      file << c.velocity[j];
      if(j + 1 < c.velocity.size())
      {
        file << ", ";
      }
    }
    file << "],\n";

    file << "      \"torque\": [";
    for(size_t j = 0; j < c.torque.size(); ++j)
    {
      file << c.torque[j];
      if(j + 1 < c.torque.size())
      {
        file << ", ";
      }
    }
    file << "]\n";

    file << "    }";
    if(i + 1 < commands_.size())
    {
      file << ",";
    }
    file << "\n";
  }
  file << "  ],\n";

  // ── Summary ──
  file << "  \"summary\": {\n";
  file << "    \"total_states_sent\": " << states_.size() << ",\n";
  file << "    \"total_commands_received\": " << commands_.size() << "\n";
  file << "  }\n";

  file << "}\n";
  file.close();

  mc_rtc::log::success("Log saved to {} ({} states, {} commands)", filename, states_.size(), commands_.size());
}

} // namespace mc_robot

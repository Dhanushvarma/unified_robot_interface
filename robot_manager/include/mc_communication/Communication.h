#pragma once

#include <mc_communication/flatbuffers/Message_generated.h>

#include <mc_rtc/Configuration.h>

#include <mutex>
#include <optional>
#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace mc_communication
{

class Communication
{
public:
  enum class MessageType
  {
    CONFIG = 0,
    STATE,
    COMMAND
  };

  Communication();

  Communication(std::string name, const mc_rtc::Configuration & com_config);

  virtual ~Communication() = default;
  Communication(const Communication &) = delete;
  Communication & operator=(const Communication &) = delete;
  Communication(Communication &&) = delete;
  Communication & operator=(Communication &&) = delete;

  virtual bool sendMessage(MessageType type, const uint8_t * data, size_t size) = 0;
  virtual bool receiveMessage(MessageType type) = 0;

  /**
   * @brief Serialize mc_rtc::Configuration to a FlatBufferBuilder
   *
   * @param config
   * @return flatbuffers::FlatBufferBuilder
   */
  static flatbuffers::FlatBufferBuilder serialize(const mc_rtc::Configuration & config);

  /**
   * @brief Deserialize data and save it to the given MessageConfig
   *
   * @param data
   * @param config
   */
  static void deserialize(const uint8_t * data, mc_rtc::Configuration & message_config);

  mc_rtc::Configuration latestConfig() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_config_.has_value() ? *latest_config_ : mc_rtc::Configuration{};
  }

protected:
  [[nodiscard]] const std::string & name() const
  {
    return name_;
  }
  [[nodiscard]] const std::string & ip() const
  {
    return ip_;
  }
  [[nodiscard]] uint16_t port() const
  {
    return port_;
  }

  void updateLatestConfig(const mc_rtc::Configuration & config)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    latest_config_ = config;
  }

private:
  const std::string name_;
  const std::string ip_;
  const uint16_t port_;

  mutable std::mutex mutex_;
  std::optional<mc_rtc::Configuration> latest_config_;
  std::optional<MessageState> latest_state_;
  std::optional<MessageCommand> latest_command_;
};

} // namespace mc_communication

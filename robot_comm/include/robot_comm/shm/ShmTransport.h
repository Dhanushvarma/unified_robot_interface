#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <mc_rtc/Configuration.h>
#include <mc_rtc/logging.h>

#include <robot_comm/serialization/Buffer.h>
#include <robot_comm/transport/TransportInterface.h>

namespace robot_comm
{

class ShmTransport : public TransportInterface
{
public:
  explicit ShmTransport(const mc_rtc::Configuration & config);

  ~ShmTransport() override;

  // ------------------------------------------------------------
  // Lifecycle
  // ------------------------------------------------------------

  void start() override;

  void stop() override;

  // ------------------------------------------------------------
  // Publish / Subscribe
  // ------------------------------------------------------------

  bool publish(const std::string & topic, const ByteBuffer & payload) override;

  void subscribe(const std::string & topic, ReceiveCallback callback) override;

  bool hasSubscriber(const std::string & topic) const override;

private:
  struct ShmPacket
  {
    std::atomic<uint64_t> sequence = 0;

    uint32_t size = 0;

    uint8_t data[65536];
  };

  struct ShmSegment
  {
    int id = -1;

    void * ptr = nullptr;

    size_t size = sizeof(ShmPacket);
  };

private:
  ShmSegment createSegment(const std::string & topic);

  key_t generateKey(const std::string & topic) const;

  void pollingLoop();

private:
  std::unordered_map<std::string, ShmSegment> segments_;

  std::unordered_map<std::string, ReceiveCallback> callbacks_;

  std::unordered_map<std::string, uint64_t> last_sequences_;

  std::atomic<bool> running_ = false;

  std::thread polling_thread_;
};

} // namespace robot_comm

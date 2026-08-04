#include <robot_comm/shm/ShmTransport.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

namespace robot_comm
{

namespace
{

constexpr size_t SHM_MAX_PAYLOAD_SIZE = 65536;

}

// ------------------------------------------------------------
// Constructor / Destructor
// ------------------------------------------------------------

ShmTransport::ShmTransport(const mc_rtc::Configuration & config) : TransportInterface(config)
{
  mc_rtc::log::info("[ShmTransport] Initialized");
}

ShmTransport::~ShmTransport()
{
  stop();

  for(auto & [topic, segment] : segments_)
  {
    if(segment.ptr && segment.ptr != reinterpret_cast<void *>(-1))
    {
      if(shmdt(segment.ptr) == -1)
      {
        perror("shmdt");
      }
    }

    if(segment.id >= 0)
    {
      if(shmctl(segment.id, IPC_RMID, nullptr) == -1)
      {
        perror("shmctl");
      }
    }
  }

  mc_rtc::log::info("[ShmTransport] Destroyed");
}

// ------------------------------------------------------------
// Lifecycle
// ------------------------------------------------------------

void ShmTransport::start()
{
  if(running_)
  {
    return;
  }

  running_ = true;

  polling_thread_ = std::thread(&ShmTransport::pollingLoop, this);

  mc_rtc::log::info("[ShmTransport] Started");
}

void ShmTransport::stop()
{
  if(!running_)
  {
    return;
  }

  running_ = false;

  if(polling_thread_.joinable())
  {
    polling_thread_.join();
  }

  mc_rtc::log::info("[ShmTransport] Stopped");
}

bool ShmTransport::publish(const std::string & topic, const ByteBuffer & payload)
{
  if(segments_.find(topic) == segments_.end())
  {
    segments_[topic] = createSegment(topic);
  }

  auto & segment = segments_.at(topic);

  auto * packet = static_cast<ShmPacket *>(segment.ptr);

  if(payload.size() > SHM_MAX_PAYLOAD_SIZE)
  {
    mc_rtc::log::error("[ShmTransport] Payload too large for topic '{}'", topic);

    return false;
  }

  std::memcpy(packet->data, payload.data(), payload.size());

  packet->size = static_cast<uint32_t>(payload.size());

  ++packet->sequence;

  return true;
}

void ShmTransport::subscribe(const std::string & topic, ReceiveCallback callback)
{
  if(hasSubscriber(topic))
  {
    return;
  }

  if(segments_.find(topic) == segments_.end())
  {
    segments_[topic] = createSegment(topic);
  }

  callbacks_[topic] = std::move(callback);

  last_sequences_[topic] = 0;

  mc_rtc::log::info("[ShmTransport] Subscribed to '{}'", topic);
}

bool ShmTransport::hasSubscriber(const std::string & topic) const
{
  return callbacks_.find(topic) != callbacks_.end();
}

ShmTransport::ShmSegment ShmTransport::createSegment(const std::string & topic)
{
  key_t key = generateKey(topic);

  int id = shmget(key, sizeof(ShmPacket), 0666 | IPC_CREAT);

  if(id < 0)
  {
    perror("shmget");

    mc_rtc::log::error_and_throw("[ShmTransport] shmget failed for '{}'", topic);
  }

  void * ptr = shmat(id, nullptr, 0);

  if(ptr == reinterpret_cast<void *>(-1))
  {
    perror("shmat");

    mc_rtc::log::error_and_throw("[ShmTransport] shmat failed for '{}'", topic);
  }

  auto * packet = static_cast<ShmPacket *>(ptr);

  packet->sequence = 0;
  packet->size = 0;

  mc_rtc::log::info("[ShmTransport] Created segment for '{}'", topic);

  return {id, ptr, sizeof(ShmPacket)};
}

key_t ShmTransport::generateKey(const std::string & topic) const
{
  std::hash<std::string> hash;

  return static_cast<key_t>(hash(topic) & 0x7FFFFFFF);
}

void ShmTransport::pollingLoop()
{
  while(running_)
  {
    for(const auto & [topic, callback] : callbacks_)
    {
      auto segment_it = segments_.find(topic);

      if(segment_it == segments_.end())
      {
        continue;
      }

      auto * packet = static_cast<ShmPacket *>(segment_it->second.ptr);

      uint64_t sequence = packet->sequence.load();

      if(sequence == last_sequences_[topic])
      {
        continue;
      }

      last_sequences_[topic] = sequence;

      ByteBuffer payload(packet->data, packet->data + packet->size);

      callback(topic, payload);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

} // namespace robot_comm

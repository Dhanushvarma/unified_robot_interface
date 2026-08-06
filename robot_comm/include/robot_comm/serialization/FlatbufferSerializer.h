#pragma once

#include <robot_comm/serialization/Serializer.h>

#include <robot_comm/serialization/flatbuffers/mc_rtc_msgs_generated.h>

namespace robot_comm
{

class FlatbufferSerializer : public ISerializer
{
public:
  SerializationBackend backend() const override
  {
    return SerializationBackend::Flatbuffer;
  }

protected:
  ByteBuffer serializeImpl(MessageType type, const void * msg) override;

  std::optional<void *> deserializeImpl(MessageType type, const uint8_t * data, size_t size) override;
};

} // namespace robot_comm

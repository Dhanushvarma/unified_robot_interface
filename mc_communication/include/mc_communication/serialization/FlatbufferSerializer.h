#pragma once

#include <mc_communication/serialization/Serializer.h>

#include <mc_communication/serialization/flatbuffers/mc_rtc_msgs_generated.h>

namespace mc_communication
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

} // namespace mc_communication

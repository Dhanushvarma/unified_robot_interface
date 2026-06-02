#pragma once

#include <optional>

#include <mc_communication/serialization/Serializer.h>
#include <mc_communication/serialization/protobuf/mc_rtc_msgs.pb.h>

namespace mc_communication
{

class ProtobufSerializer : public ISerializer
{
public:
  SerializationBackend backend() const override
  {
    return SerializationBackend::Protobuf;
  }

protected:
  ByteBuffer serializeImpl(MessageType type, const void * msg) override;

  std::optional<void *> deserializeImpl(MessageType type, const uint8_t * data, size_t size) override;

private:
  template<typename T>
  ByteBuffer serializeMessage(const T & msg)
  {
    ByteBuffer buffer(msg.ByteSizeLong());

    msg.SerializeToArray(buffer.data(), static_cast<int>(buffer.size()));

    return buffer;
  }

  template<typename T>
  std::optional<T> deserializeMessage(const uint8_t * data, size_t size)
  {
    T msg;

    if(!msg.ParseFromArray(data, static_cast<int>(size)))
    {
      return std::nullopt;
    }

    return msg;
  }
};

} // namespace mc_communication

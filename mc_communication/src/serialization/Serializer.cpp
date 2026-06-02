#include <mc_communication/serialization/Serializer.h>

namespace mc_communication
{

template<typename T>
ByteBuffer ISerializer::serialize(const T & msg)
{
  return serializeImpl(MessageTraits<T>::type, &msg);
}

template<typename T>
std::optional<T> ISerializer::deserialize(MessageType type, const uint8_t * data, size_t size)
{
  auto result = deserializeImpl(type, data, size);

  if(!result)
  {
    return std::nullopt;
  }

  return *static_cast<T *>(*result);
}

} // namespace mc_communication

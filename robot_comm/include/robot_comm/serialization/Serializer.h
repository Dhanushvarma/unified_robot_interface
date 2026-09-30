#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>

#include "Buffer.h"
#include "MessageTraits.h"

namespace robot_comm
{

/// Converts messages to and from bytes. Implement serializeImpl() and
/// deserializeImpl() to add a serialization backend.
class ISerializer
{
protected:
  virtual ByteBuffer serializeImpl(MessageType type, const void * msg) = 0;

  virtual std::optional<void *> deserializeImpl(MessageType type, const uint8_t * data, size_t size) = 0;

public:
  virtual ~ISerializer() = default;

  virtual SerializationBackend backend() const = 0;

  template<typename T>
  ByteBuffer serialize(const T & msg)
  {
    return serializeImpl(MessageTraits<T>::type, &msg);
  }

  template<typename T>
  std::optional<T> deserialize(MessageType type, const uint8_t * data, size_t size)
  {
    auto result = deserializeImpl(type, data, size);

    if(!result)
    {
      return std::nullopt;
    }

    return *static_cast<T *>(*result);
  }
};

} // namespace robot_comm

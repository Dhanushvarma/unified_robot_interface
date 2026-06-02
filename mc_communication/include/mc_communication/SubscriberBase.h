#pragma once

#include <mc_communication/serialization/Buffer.h>

namespace mc_communication
{
class SubscriberBase
{
public:
  SubscriberBase(const std::string & topic) : topic_(topic) {};
  virtual ~SubscriberBase() = default;

  virtual void receive(const ByteBuffer & payload) = 0;

  inline const std::string & topic() const
  {
    return topic_;
  };

private:
  std::string topic_;
};
} // namespace mc_communication

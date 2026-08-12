#pragma once

#include <robot_comm/serialization/Buffer.h>

#include <string>

namespace robot_comm
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

} // namespace robot_comm

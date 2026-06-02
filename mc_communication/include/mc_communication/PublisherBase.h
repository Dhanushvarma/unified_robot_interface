#pragma once

#include <mc_communication/serialization/Buffer.h>

namespace mc_communication
{

class CommunicationContext;

class PublisherBase
{
public:
  PublisherBase(CommunicationContext & ctx, std::string topic) : ctx_(ctx), topic_(std::move(topic)) {}

  virtual ~PublisherBase() = default;

  virtual bool publish(const ByteBuffer & payload) = 0;

  const std::string & topic() const
  {
    return topic_;
  }

protected:
  CommunicationContext & context()
  {
    return ctx_;
  }

private:
  CommunicationContext & ctx_;
  std::string topic_;
};
} // namespace mc_communication

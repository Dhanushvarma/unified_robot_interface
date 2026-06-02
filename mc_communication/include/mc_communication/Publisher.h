#pragma once

#include <mc_communication/PublisherBase.h>
#include <mc_communication/serialization/FlatbufferSerializer.h>

namespace mc_communication
{

template<typename T>
class Publisher : public PublisherBase
{
public:
  Publisher(CommunicationContext & ctx, std::string topic) : PublisherBase(ctx, std::move(topic)) {}

  bool publish(const T & msg)
  {
    auto payload = context().serializer().serialize(T::TYPE, &msg);

    return publish(payload);
  }

  bool publish(const ByteBuffer & payload) override
  {
    return context().transport().publish(topic(), payload);
  }
};

template<typename T, typename SerializerT = FlatbufferSerializer, typename... Args>
std::shared_ptr<Publisher<T>> make_publisher(const std::string & topic, Args &&... args)
{
  auto serializer = std::make_shared<SerializerT>(std::forward<Args>(args)...);

  return std::make_shared<Publisher<T>>(topic, serializer);
}
} // namespace mc_communication

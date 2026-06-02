#include <mc_communication/SubscriberBase.h>
#include <mc_communication/serialization/Serializer.h>

namespace mc_communication
{
template<typename T>
class Subscriber : public SubscriberBase
{
public:
  using Callback = std::function<void(const T &)>;

public:
  Subscriber(const std::string & topic, const std::shared_ptr<ISerializer> & serializer, const Callback & cb)
  : SubscriberBase(topic), serializer_(serializer), callback_(cb)
  {
  }

  void receive(const ByteBuffer & payload) override
  {
    auto msg = serializer_->deserialize<T>(MessageTraits<T>::type, payload.data(), payload.size());

    if(msg)
    {
      callback_(*msg);
    }
  }

private:
  std::shared_ptr<ISerializer> serializer_;

  Callback callback_;
};
} // namespace mc_communication

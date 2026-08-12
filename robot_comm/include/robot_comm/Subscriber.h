#include <robot_comm/SubscriberBase.h>
#include <robot_comm/serialization/Serializer.h>

#include <functional>
#include <memory>
#include <string>

namespace robot_comm
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

} // namespace robot_comm

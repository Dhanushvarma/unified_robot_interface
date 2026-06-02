#include <mc_communication/zenoh/ZenohCommunication.h>

namespace mc_communication
{

ZenohCommunication::ZenohCommunication(std::string name, const mc_rtc::Configuration & config)
: Communication(std::move(name), config)
{
  transport_ = std::make_shared<ZenohTransport>(config);
}

} // namespace mc_communication

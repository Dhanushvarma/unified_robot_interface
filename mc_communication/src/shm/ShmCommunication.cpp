#include <mc_communication/shm/ShmCommunication.h>

namespace mc_communication
{

ShmCommunication::ShmCommunication(std::string name, const mc_rtc::Configuration & config)
: Communication(std::move(name), config)
{
  transport_ = std::make_shared<ShmTransport>(config);
}

} // namespace mc_communication

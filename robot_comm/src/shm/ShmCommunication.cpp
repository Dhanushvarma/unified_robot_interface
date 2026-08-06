#include <robot_comm/shm/ShmCommunication.h>

namespace robot_comm
{

ShmCommunication::ShmCommunication(std::string name, const mc_rtc::Configuration & config)
: Communication(std::move(name), config)
{
  transport_ = std::make_shared<ShmTransport>(config);
}

} // namespace robot_comm

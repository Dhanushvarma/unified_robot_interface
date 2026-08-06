#include <robot_comm/CommunicationFactory.h>
#include <robot_comm/zenoh/ZenohCommunication.h>
// #include <robot_comm/shm/ShmCommunication.h>
// #include <robot_comm/tcp/TcpCommunication.h>

#include <mc_rtc/logging.h>

namespace robot_comm
{

std::unique_ptr<Communication> CommunicationFactory::makeCommunication(const std::string & name,
                                                                       const mc_rtc::Configuration & config)
{
  const std::string protocol = config("protocol");

  if(protocol == "zenoh" || protocol == "zenoh/shm")
  {
    return std::make_unique<ZenohCommunication>(name, config);
  }
  // if(protocol == "tcp") { return std::make_unique<TcpCommunication>(name, config); }
  // if(protocol == "shm") { return std::make_unique<ShmCommunication>(name, config); }

  mc_rtc::log::error_and_throw("Communication protocol '{}' is not supported", protocol);
  return nullptr;
}

} // namespace robot_comm

#include <mc_communication/CommunicationFactory.h>
#include <mc_communication/zenoh/ZenohCommunication.h>
// #include <mc_communication/shm/ShmCommunication.h>

#include <mc_rtc/logging.h>

namespace mc_communication
{

std::unique_ptr<Communication> CommunicationFactory::makeCommunication(const std::string & name,
                                                                       const mc_rtc::Configuration & com_config)
{
  const std::string protocol = com_config("protocol");

  if(protocol == "zenoh/shm")
  {
    mc_rtc::log::info("PROTOCOL:{}", protocol);
    return std::make_unique<ZenohCommunication>(name, com_config);
  }
  // else if(protocol == "shm")
  // {
  //   return std::make_unique<ShmCommunication>(name, com_config);
  // }

  mc_rtc::log::error_and_throw("Communication protocol {} is not supported, currently supported (zenoh)", protocol);
  return nullptr;
}

} // namespace mc_communication

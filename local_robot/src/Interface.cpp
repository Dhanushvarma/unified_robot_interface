#include <Interface.h>

#include <mc_rtc/logging.h>

namespace mc_interface
{

RobotInterfaceTemplate::RobotInterfaceTemplate()
{
  mc_rtc::log::success("RobotInterfaceTemplate start");
  // TODO:
  // from etc/mc_rtc.yaml watchout for new information there
  // when receive, start the interface
  mc_rtc::log::info("RobotInterfaceTemplate done");
};

RobotInterfaceTemplate::RobotInterfaceTemplate(const std::string & name,
                                               const mc_rtc::Configuration & config,
                                               uint8_t buffer_size)
: RobotInterface(name, config, buffer_size)
{
  mc_rtc::log::success("RobotInterfaceTemplate start");
  mc_rtc::log::info("RobotInterfaceTemplate done");
};

} // namespace mc_interface

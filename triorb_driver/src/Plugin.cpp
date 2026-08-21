#include <triorb_driver/RobotDriverTriOrb.h>

#include <cstdint>
#include <string>
#include <vector>

extern "C"
{

  void MC_RTC_ROBOT_DRIVER(std::vector<std::string> & classes)
  {
    classes.push_back("RobotDriverTriOrb");
  }

  mc_robot_interface::RobotDriver * create(const std::string &, const std::string & ip, const uint16_t & port)
  {
    return new triorb_driver::RobotDriverTriOrb(ip, port);
  }

  void destroy(mc_robot_interface::RobotDriver * driver)
  {
    delete driver;
  }
}

#include <mc_communication/CommunicationZenoh.h>
#include <mc_rtc/logging.h>
#include <mc_robot_manager/RobotManager.h>

#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace mc_fleet
{

void RobotManager::processGConfig(mc_control::MCGlobalController::GlobalConfiguration & gconfig)
{
  mc_rtc::log::success("manager processGConfig start");

  if(!gconfig.config.has("Robots"))
  {
    mc_rtc::log::error_and_throw<std::runtime_error>(
        "No `Robots` section in the configuration, see etc/mc_rtc.yaml for an example");
  }

  mc_rtc::log::info("manager processGConfig 1");

  if(gconfig.config.has("Default"))
  {
    mc_rtc::Configuration dc = gconfig.config("Default");
    user_default_.module = dc("module", std::string(user_default_.module));
    user_default_.control_mode = dc("control_mode", std::string(user_default_.control_mode));
    user_default_.driver = dc("driver", std::string(user_default_.driver));
    user_default_.time_step = dc("time_step", double(user_default_.time_step));
    user_default_.communication_protocol = dc("communication", std::string(user_default_.communication_protocol));
  }

  mc_rtc::log::info("manager processGConfig 2");

  mc_rtc::Configuration robots_config = gconfig.config("Robots");
  for(auto & robot_name : robots_config.keys())
  {
    mc_rtc::Configuration robot_config{gconfig.config("Robots")(robot_name)};

    if(robot_config.has("base"))
    {
      mc_rtc::Configuration base_config{};
      base_config.load(robots_config(robot_config("base")));
      base_config.load(robot_config);
      robot_config.load(base_config);
    }

    if(!robot_config.has("module"))
    {
      robot_config.add("module", user_default_.module);
    }

    if(!robot_config.has("controller"))
    {
      robot_config.add("controller");
      robot_config("controller").add("mode", user_default_.control_mode);
      robot_config("controller").add("driver", user_default_.driver);
      robot_config("controller").add("time_step", user_default_.time_step);
    }
    else
    {
      if(!robot_config("controller").has("mode"))
      {
        robot_config("controller").add("mode", user_default_.control_mode);
      }
      if(!robot_config("controller").has("driver"))
      {
        robot_config("controller").add("driver", user_default_.driver);
      }
      if(!robot_config("controller").has("time_step"))
      {
        robot_config("controller").add("time_step", user_default_.time_step);
      }
    }

    if(robot_config.has("communication"))
    {
      if(!robot_config("communication").has("protocol"))
      {
        robot_config("communication").add("protocol", user_default_.communication_protocol);
      }
    }
    else
    {
      mc_rtc::log::error_and_throw("No `communication` section in the configuration of robot {}", robot_name);
    }
  }

  mc_rtc::log::info("manager processGConfig done");
}

void RobotManager::init()
{
  mc_rtc::log::success("manager init start");

  mc_rtc::Configuration robots_config = gconfig_.config("Robots");
  mc_rtc::log::info(robots_config.dump(true, true));

  /* Set up robot interface and communication*/
  for(auto & robot_name : robots_config.keys())
  {
    mc_rtc::log::info("manager init robot {}", robot_name);

    if(interfaces_.count(robot_name) != 0)
    {
      mc_rtc::log::error("Skip already exists robot interface {}", robot_name);
      continue;
    }

    mc_rtc::Configuration robot_config{robots_config(robot_name)};
    std::unique_ptr<mc_robot::RobotInterface> interface =
        mc_robot::RobotInterfaceFactory::makeInterface(robot_name, robot_config);
    if(!interface)
    {
      continue;
    }

    interfaces_.try_emplace(robot_name, std::move(interface));
  }

  /* Send config to real robots */
  for(auto & [robot_name, interface] : interfaces_)
  {
    mc_rtc::log::info("manager init send config {}", robot_name);

    auto builder = mc_communication::Communication::serialize(interface->config());

    interface->communication().sendMessage(mc_communication::Communication::MessageType::CONFIG,
                                           builder.GetBufferPointer(), builder.GetSize());
  }

  mc_rtc::log::info("manager init done");
}

} // namespace mc_fleet

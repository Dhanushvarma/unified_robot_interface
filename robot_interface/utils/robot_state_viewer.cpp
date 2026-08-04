#include <robot_interface/PluginLoader.h>
#include <robot_interface/RobotDriverTemplate.h>
#include <robot_interface/config.h>

#include <mc_rtc/Configuration.h>
#include <mc_rtc/logging.h>

#include <CLI/CLI.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::atomic<bool> g_interrupt{false};

static void signal_handler(int /*sig*/)
{
  g_interrupt = true;
}

static std::string formatRow(int idx, double pos, double vel, double torque)
{
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(4);
  ss << "  " << idx << "   │ " << std::setw(12) << pos << "   │ " << std::setw(12) << vel << "   │ " << std::setw(12)
     << torque;
  return ss.str();
}

static void printState(const std::string & driver_name,
                       const std::string & ip,
                       const std::vector<double> & q,
                       const std::vector<double> & qd,
                       const std::vector<double> & tau)
{
  // Move cursor to top-left and clear screen.
  std::cout << "\033[H\033[2J";

  std::cout << "Robot State Viewer — " << driver_name << " @ " << ip << "\n";
  std::cout << std::string(60, '-') << "\n";
  std::cout << "Joint │  Position (rad)  │  Velocity (rad/s)  │  Torque (Nm)\n";
  std::cout << "──────┼──────────────────┼────────────────────┼─────────────\n";

  const size_t n = q.size();
  for(size_t i = 0; i < n; ++i)
  {
    double vel = (i < qd.size()) ? qd[i] : 0.0;
    double tor = (i < tau.size()) ? tau[i] : 0.0;
    std::cout << formatRow(static_cast<int>(i), q[i], vel, tor) << "\n";
  }

  std::cout << std::string(60, '-') << "\n";
  std::cout << "Press Ctrl+C to exit.\n";
  std::cout.flush();
}

int main(int argc, char ** argv)
{
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  CLI::App app{"Display live joint state from a robot driver plugin"};
  std::string config_path;
  double display_rate_hz = 10.0;
  app.add_option("-c,--config", config_path, "Path to robot interface config file (YAML)")->required();
  app.add_option("-r,--rate", display_rate_hz, "Display refresh rate in Hz")->default_val(10.0);
  CLI11_PARSE(app, argc, argv);
  const auto display_period = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
      std::chrono::duration<double>(1.0 / display_rate_hz));

  mc_rtc::Configuration config(config_path);

  const std::string driver_name = config("robot_interface")("driver", std::string{});
  if(driver_name.empty())
  {
    mc_rtc::log::error_and_throw("'driver' key missing from config '{}'", config_path);
  }
  const std::string ip = config("robot_interface")("ip", std::string{"127.0.0.1"});
  const uint16_t port = config("robot_interface")("port", uint16_t{0});

  mc_rtc::log::info("[robot_state_viewer] Loading driver '{}' ({}:{})", driver_name, ip, port);

  mc_robot_interface::PluginLoader<mc_robot_interface::RobotDriver> loader(
      "MC_RTC_ROBOT_DRIVER", {mc_robot_interface::MC_ROBOT_INTERFACE_INSTALL_PREFIX});
  std::shared_ptr<mc_robot_interface::RobotDriver> driver;

  try
  {
    driver = loader.load(driver_name, ip, port);
  }
  catch(const std::exception & e)
  {
    mc_rtc::log::error("[robot_state_viewer] Failed to load driver: {}", e.what());
    return 1;
  }

  mc_rtc::log::info("[robot_state_viewer] Connected. Starting state display at {:.1f} Hz.", display_rate_hz);

  auto last_display = std::chrono::steady_clock::now() - display_period;

  while(!g_interrupt)
  {
    driver->sync();

    const auto now = std::chrono::steady_clock::now();
    if(now - last_display >= display_period)
    {
      last_display = now;
      printState(driver_name, ip, driver->getActualQ(), driver->getActualQd(), driver->getJointTorques());
    }
  }

  std::cout << "\n[robot_state_viewer] Exiting.\n";
  return 0;
}

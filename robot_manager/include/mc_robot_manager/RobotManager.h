#pragma once

#include <mc_communication/CommunicationFactory.h>
#include <mc_robot_interface/RobotInterfaceFactory.h>

#include <mc_control/mc_global_controller.h>

#include <atomic>
#include <condition_variable>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

namespace mc_fleet
{

void run(void * data, const std::atomic<bool> & interrupt);

void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt);

class RobotManager
{
public:
  RobotManager();

  RobotManager(const std::string & mc_config_path, std::string com_config_path, const std::atomic<bool> & interrupt);

  RobotManager(const std::string & mc_config_path, const std::atomic<bool> & interrupt);

  ~RobotManager();

  RobotManager(const RobotManager &) = delete;
  RobotManager & operator=(const RobotManager &) = delete;
  RobotManager(RobotManager &&) = delete;
  RobotManager & operator=(RobotManager &&) = delete;

  [[nodiscard]] mc_control::MCGlobalController & gcontroller()
  {
    return *gcontroller_;
  }

  [[nodiscard]] const auto & interfaces()
  {
    return interfaces_;
  }

  void notify()
  {
    cv_.notify_one();
  }

private:
  void init(const std::atomic<bool> & interrupt);

  std::unique_ptr<mc_control::MCGlobalController> gcontroller_;

  std::unordered_map<std::string, std::unique_ptr<mc_robot::RobotInterfaceBase>> interfaces_{};
  const std::string com_config_path_;

  /* Process configuration */
  void processGConfig(mc_control::MCGlobalController::GlobalConfiguration & gconfig);
  mc_control::MCGlobalController::GlobalConfiguration gconfig_;
  struct DefaultConfig
  {
    std::string module{};
    std::string control_mode{"position"};
    std::string driver{};
    double time_step{0.001};
    std::string communication_protocol{"zenoh"};
  };
  DefaultConfig user_default_;

  /* Start */
  std::unique_ptr<std::thread> main_thread_;
  std::mutex start_mutex_;
  std::condition_variable start_cv_;
  bool start_control_{false};

  /* Multi-threading */
  std::condition_variable cv_;
  std::vector<std::thread> threads_;
  void mainThread(size_t step_size, const std::atomic<bool> & interrupt);

  /* Replace tools */
  struct ReplaceRequest
  {
    std::string old_robot_name;
    std::string new_robot_name;
  };
  mc_rtc::Slot<std::string, std::string> replace_slot_;
  std::queue<ReplaceRequest> replace_queue_;
  bool signal_received_ = false;
  mutable std::mutex replace_mutex_;

  // TODO idea here is to have a thread for each communication we are lauching for each robot
  // Would be nice to consider the case where robot are sharing the same timestep and protocol
  // Each thread is running at each robot dt.
  // Warning should be set in case the dt outreach protocol capacities
};

} // namespace mc_fleet

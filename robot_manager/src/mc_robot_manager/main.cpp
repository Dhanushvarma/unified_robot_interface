#include <mc_rtc/logging.h>
#include <mc_robot_manager/RobotManager.h>

// Resolve redefinition conflict with pthread.h
#define sched_param linux_sched_param // NOLINT(readability-identifier-naming)
#include <linux/sched.h>
#include <linux/sched/types.h>
#undef sched_param

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/types.h>
#include <syscall.h>
#include <unistd.h>

int schedSetattr(pid_t pid, const struct sched_attr * attr, unsigned int flags)
{
  return static_cast<int>(syscall(__NR_sched_setattr, pid, attr, flags)); // NOLINT(cppcoreguidelines-pro-type-vararg)
}

namespace
{
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<bool> interrupt{false};
} // namespace

void signalHandler(int s)
{
  mc_rtc::log::warning("Caught signal {}", s);
  interrupt = true;
}

int main(int argc, char * argv[])
{
  signal(SIGINT, signalHandler);

  /* Lock Memory*/
  if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
  {
    mc_rtc::log::error("mlockall failed: {}", strerror(errno));
    if(errno == ENOMEM)
    {
      mc_rtc::log::info("Check /etc/security/limits.conf for memlock limits.");
    }
    return -2;
  }

  uint64_t cycle_ns{1000UL * 1000UL}; // 1 ms default cycle
  const char * mc_rt_freq = getenv("MC_RT_FREQ");
  if(mc_rt_freq != nullptr)
  {
    cycle_ns = static_cast<uint64_t>(atoi(mc_rt_freq)) * 1000UL * 1000UL;
  }

  /* Initialize callback (non real-time yet) */
  void * raw = mc_fleet::init(argc, argv, cycle_ns, interrupt);
  if(raw == nullptr)
  {
    mc_rtc::log::error("Initialization failed");
    return -2;
  }

  // Automatically free data if schedSetattr fails
  std::unique_ptr<mc_fleet::RobotManager> robot_manager{static_cast<mc_fleet::RobotManager *>(raw)};

  /* Time reservation */
  struct sched_attr attr = {};
  memset(&attr, 0, sizeof(attr));
  attr.size = sizeof(attr);
  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_runtime = attr.sched_deadline = attr.sched_period = cycle_ns; // nanoseconds

  mc_rtc::log::info("Running thread at {}ms per cycle", double(cycle_ns) / 1e6);

  /* Set scheduler policy for the main thread */
  if(schedSetattr(0, &attr, 0) < 0)
  {
    mc_rtc::log::error("schedSetattr failed");
    return -2;
  }

  /* Run */
  mc_fleet::run(robot_manager.get(), interrupt);

  return 0;
}

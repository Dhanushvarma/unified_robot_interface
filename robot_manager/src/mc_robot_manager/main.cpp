#include <mc_robot_manager/RobotManager.h>

#include <fmt/core.h>
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
#include <cstring>
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
// Use atomic to prevent cached reading
std::atomic<bool> interrupt{false};
} // namespace

void signalHandler(int s)
{
  fmt::print("[warning] Caught signal {}\n", s);
  interrupt = true;
}

int main(int argc, char * argv[])
{
  signal(SIGINT, signalHandler);

  /* Lock Memory */
  if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
  {
    fmt::print("[error] mlockall failed: {}\n", std::strerror(errno));
    if(errno == ENOMEM)
    {
      fmt::print("Check /etc/security/limits.conf for memlock limits.\n");
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
    fmt::print("[error] Initialization failed\n");
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

  fmt::print("Running thread at {}ms per cycle\n", double(cycle_ns) / 1e6);

  /* Set scheduler policy for the main thread */
  if(schedSetattr(0, &attr, 0) < 0)
  {
    fmt::print("[error] schedSetattr failed\n");
    // return -2;
  }

  /* Run */
  mc_fleet::run(robot_manager.get(), interrupt);

  return 0;
}

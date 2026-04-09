#include <mc_rtc/logging.h>
#include <mc_robot_manager/RobotManager.h>

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

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
  }

  uint64_t cycle_ns{1000UL * 1000UL}; // 1 ms default cycle
  const char * mc_rt_freq = getenv("MC_RT_FREQ");
  if(mc_rt_freq != nullptr)
  {
    cycle_ns = static_cast<uint64_t>(atoi(mc_rt_freq)) * 1000UL * 1000UL;
  }

  /* Initialize callback (non real-time yet) */
  void * data = mc_fleet::init(argc, argv, cycle_ns, interrupt);
  if(data == nullptr)
  {
    mc_rtc::log::error("Initialization failed");
    return -2;
  }

  mc_rtc::log::info("Running thread at {}ms per cycle", double(cycle_ns) / 1e6);

  /* Run */
  mc_fleet::run(data, interrupt);

  return 0;
}

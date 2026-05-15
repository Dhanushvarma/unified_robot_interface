#include <InterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

// Resolve redefinition conflict with pthread.h
#define sched_param linux_sched_param // NOLINT(readability-identifier-naming)
#include <linux/sched.h>
#include <linux/sched/types.h>
#undef sched_param

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
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
  mc_rtc::log::warning("[mc_local] Caught signal {}", s);
  interrupt = true;
}

int main(int argc, char * argv[])
{
  signal(SIGINT, signalHandler);

  /* Lock Memory*/
  if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
  {
    mc_rtc::log::error("[mc_local] mlockall failed: {}", strerror(errno));
    if(errno == ENOMEM)
    {
      mc_rtc::log::info("[mc_local] Check /etc/security/limits.conf for memlock limits.");
    }
  }

  uint64_t cycle_ns{1000UL * 1000UL}; // 1 ms default cycle
  const char * mc_rt_freq = getenv("MC_RT_FREQ");
  if(mc_rt_freq != nullptr)
  {
    cycle_ns = static_cast<uint64_t>(atoi(mc_rt_freq)) * 1000UL * 1000UL;
  }

  /* Initialize callback (non real-time yet) */
  void * raw = mc_interface_template::init(argc, argv, cycle_ns, interrupt);
  if(raw == nullptr)
  {
    mc_rtc::log::error("[mc_local] Initialization failed");
    return -2;
  }

  std::unique_ptr<mc_interface_template::InterfaceTemplate> data // Automatically free data if schedSetattr fails
      {static_cast<mc_interface_template::InterfaceTemplate *>(raw)};

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
  mc_interface_template::run(data.get(), interrupt);

  return 0;
}

namespace mc_interface_template
{

void run(void * data, const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("local run start");
  auto * interface_template = static_cast<InterfaceTemplate *>(data);

  while(!interrupt)
  {
    interface_template->updateSensors();
    interface_template->updateControl();
  }

  mc_rtc::log::info("local run done");
}

void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("local init start");

  std::string conf_file;
  po::options_description desc("mc_interface_template options");
  // clang-format off
   desc.add_options()
    ("help,h", "Display help message")
    ("conf,f", po::value<std::string>(&conf_file), "Configuration file");
  // clang-format on

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if(vm.count("help") != 0U)
  {
    std::cout << desc << "\n";
    std::cout << "see etc/mc_rtc.yaml for example configuration\n";
    return nullptr;
  }

  if(vm.count("conf") != 0U)
  {
    mc_rtc::log::error("'conf' is not supported");
    return nullptr;
  }

  mc_rtc::log::info("local init 2");

  /* Initialize robot manager */
  auto interface = std::make_unique<mc_interface_template::InterfaceTemplate>(interrupt);

  mc_rtc::log::info("local init done");

  return interface.release();
}

} // namespace mc_interface_template

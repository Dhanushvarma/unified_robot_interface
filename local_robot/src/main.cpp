#include <mc_communication/CommunicationFactory.h>
#include <InterfaceTemplate.h>

#include <mc_rtc/logging.h>
#include <zenoh.hxx>

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
#include <thread>
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

  // Automatically free data if schedSetattr fails
  std::unique_ptr<mc_communication::Communication> data{static_cast<mc_communication::Communication *>(raw)};

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
    // return -2;
  }

  /* Run */
  mc_interface_template::run(data.get(), interrupt);

  return 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// --- MC_INTERFACE_TEMPLATE -------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

namespace mc_interface_template
{

void run(void * data, const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("local run start");
  auto * interface = static_cast<mc_communication::Communication *>(data);

  /* QUERY FOR CONFIGURATION */
  mc_rtc::log::info("[mc_communication] Waiting for config...");
  while(!interrupt)
  {
    auto config = interface->query<std::string>("robot2_zenoh");

    if(config)
    {
      mc_rtc::log::success("[mc_communication] Got config from server");
      mc_rtc::log::info(*config);
      // TODO: parse *config here if needed and apply it
      break;
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
  }

  while(!interrupt)
  {
    /* updateSensors(); */
    /* updateControl(); */
  }

  mc_rtc::log::info("local run done");
}

void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("local init start");

  std::string conf_path;
  po::options_description desc("mc_interface_template options");
  // clang-format off
   desc.add_options()
    ("help,h", "Display help message")
    ("conf,f", po::value<std::string>(&conf_path), "Configuration file");
  // clang-format on

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if(conf_path.empty())
  {
    std::exit(0);
  }

  mc_rtc::Configuration conf_file(conf_path);

  mc_rtc::log::warning("[CLIENT] Creating with config:");
  mc_rtc::log::warning("  protocol = {}", conf_file("local_robot")("protocol").operator std::string());
  if(conf_file("local_robot").has("configuration"))
  {
    mc_rtc::log::warning("  config file = {}", conf_file("local_robot")("configuration").operator std::string());
  }

  // Wait for router to start communication
  std::unique_ptr<mc_communication::Communication> interface;
  while(!interrupt)
  {
    try
    {
      interface = mc_communication::CommunicationFactory::makeCommunication("client", conf_file("local_robot"));
      break;
    }
    catch(const std::exception & e)
    {
      mc_rtc::log::warning("Waiting for router... ({})", e.what());
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  }

  mc_rtc::log::info("local init done");

  return interface.release();
}

} // namespace mc_interface_template

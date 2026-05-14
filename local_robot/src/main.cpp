#include <InterfaceTemplate.h>

#include <mc_rtc/logging.h>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <atomic>
// #include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
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
  void * data = mc_interface_template::init(argc, argv, cycle_ns, interrupt);
  if(data == nullptr)
  {
    mc_rtc::log::error("[mc_local] Initialization failed");
    return -2;
  }

  /* Run */
  mc_interface_template::run(data, interrupt);

  return 0;
}

namespace mc_interface_template
{

void run(void * data, const std::atomic<bool> & interrupt)
{
  mc_rtc::log::success("local run start");
  std::unique_ptr<InterfaceTemplate> interface_template{static_cast<InterfaceTemplate *>(data)};

  while(!interrupt)
  {
    interface_template->updateSensors();
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

// TODO: add guard to exclude launch zenoh router if not needed
#include <config.h>

#include <gtest/gtest.h>
#include <zenoh.hxx>

#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

namespace fs = std::filesystem;

class ZenohRouterEnvironment : public ::testing::Environment
{
public:
  void SetUp() override
  {
    zenoh::Config router_config = zenoh::Config::from_file(fs::path(TEST_CONFIG_DIR) / "zenoh/router.json5");

    router_ = std::make_unique<zenoh::Session>(zenoh::Session::open(std::move(router_config)));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  void TearDown() override
  {
    router_.reset();
  }

private:
  std::unique_ptr<zenoh::Session> router_;
};

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  // Register the environment globally once for the entire binary execution
  ::testing::AddGlobalTestEnvironment(new ZenohRouterEnvironment);
  return RUN_ALL_TESTS();
}

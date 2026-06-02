#include <filesystem>
#include <gtest/gtest.h>

#include <mc_communication/CommunicationFactory.h>
#include "config.h"

namespace fs = std::filesystem;

using namespace mc_communication;

TEST(CommunicationFactory, CreateServer)
{
  mc_rtc::Configuration config(fs::path(TEST_CONFIG_DIR) / "test.yaml");

  EXPECT_NO_THROW({
    auto server = CommunicationFactory::makeCommunication("server", config("Robots")("robot1_zenoh")("communication"));

    EXPECT_NE(server, nullptr);
  });
}

TEST(CommunicationFactory, CreateClient)
{
  mc_rtc::Configuration config(fs::path(TEST_CONFIG_DIR) / "test.yaml");

  EXPECT_NO_THROW({
    auto client = CommunicationFactory::makeCommunication("robot3", config("Robots")("robot1_zenoh")("communication"));

    EXPECT_NE(client, nullptr);
  });
}

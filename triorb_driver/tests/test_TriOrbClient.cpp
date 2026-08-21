#include <triorb_driver/TriOrbClient.h>

#include <gtest/gtest.h>

#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{

class FakeHttpTransport final : public triorb_driver::HttpTransport
{
public:
  struct Request
  {
    std::string method;
    std::string path;
    std::string body;
  };

  triorb_driver::HttpResponse get(const std::string & path) override
  {
    requests.push_back({"GET", path, {}});

    if(responses.empty())
    {
      return {500, R"({"detail":"no fake response"})"};
    }

    auto response = responses.front();
    responses.pop_front();
    return response;
  }

  triorb_driver::HttpResponse post(const std::string & path, const std::string & body) override
  {
    requests.push_back({"POST", path, body});

    if(responses.empty())
    {
      return {500, R"({"detail":"no fake response"})"};
    }

    auto response = responses.front();
    responses.pop_front();
    return response;
  }

  std::deque<triorb_driver::HttpResponse> responses;
  std::vector<Request> requests;
};

} // namespace

TEST(TriOrbClient, HealthUsesExpectedEndpoint)
{
  auto transport = std::make_unique<FakeHttpTransport>();

  auto * fake = transport.get();

  fake->responses.push_back({200, R"({"status":"ok"})"});

  triorb_driver::TriOrbClient client(std::move(transport));

  EXPECT_NO_THROW(client.checkHealth());

  ASSERT_EQ(fake->requests.size(), 1);
  EXPECT_EQ(fake->requests[0].method, "GET");
  EXPECT_EQ(fake->requests[0].path, "/system/health");
}

TEST(TriOrbClient, ReadsVslamPose)
{
  auto transport = std::make_unique<FakeHttpTransport>();

  auto * fake = transport.get();

  fake->responses.push_back({200,
                             R"({
            "robot_position": {
              "x": 1.25,
              "y": -0.5,
              "theta_deg": 90.0
            }
          })"});

  triorb_driver::TriOrbClient client(std::move(transport));

  const auto pose = client.readPose();

  ASSERT_TRUE(pose.valid);
  EXPECT_DOUBLE_EQ(pose.x, 1.25);
  EXPECT_DOUBLE_EQ(pose.y, -0.5);

  // Web API yaw is clockwise-positive degrees.
  // Internal driver yaw is counterclockwise-positive radians.
  EXPECT_NEAR(pose.theta, -1.5707963267948966, 1e-12);

  ASSERT_EQ(fake->requests.size(), 1);
  EXPECT_EQ(fake->requests[0].path, "/vslam/robot/position");
}

TEST(TriOrbClient, RejectsMalformedPoseResponse)
{
  auto transport = std::make_unique<FakeHttpTransport>();

  auto * fake = transport.get();

  fake->responses.push_back({200, R"({"robot_position":{"x":1.0}})"});

  triorb_driver::TriOrbClient client(std::move(transport));

  EXPECT_THROW(client.readPose(), std::runtime_error);
}

TEST(TriOrbClient, ReportsMissingVslamPose)
{
  auto transport = std::make_unique<FakeHttpTransport>();

  auto * fake = transport.get();

  fake->responses.push_back({404, R"({"detail":"pose unavailable"})"});

  triorb_driver::TriOrbClient client(std::move(transport));

  const auto pose = client.readPose();

  EXPECT_FALSE(pose.valid);
}

TEST(TriOrbClient, SendsBodyFrameVelocity)
{
  auto transport = std::make_unique<FakeHttpTransport>();

  auto * fake = transport.get();

  fake->responses.push_back({200, R"({"message":"accepted"})"});

  triorb_driver::TriOrbClient client(std::move(transport));

  EXPECT_NO_THROW(client.setBodyVelocity(0.1, -0.2, 0.3));

  ASSERT_EQ(fake->requests.size(), 1);
  EXPECT_EQ(fake->requests[0].method, "POST");
  EXPECT_EQ(fake->requests[0].path, "/control/drive/run_vel");

  const auto json = nlohmann::json::parse(fake->requests[0].body);

  // These assertions intentionally keep the API-specific
  // request schema in one test.
  ASSERT_TRUE(json.contains("velocity"));
  EXPECT_DOUBLE_EQ(json.at("velocity").at("x").get<double>(), 0.1);
  EXPECT_DOUBLE_EQ(json.at("velocity").at("y").get<double>(), -0.2);
  EXPECT_DOUBLE_EQ(json.at("velocity").at("w").get<double>(), 0.3);
}

TEST(TriOrbClient, WakeSleepAndStopUseExpectedEndpoints)
{
  auto transport = std::make_unique<FakeHttpTransport>();

  auto * fake = transport.get();

  fake->responses.push_back({200, "{}"});
  fake->responses.push_back({200, "{}"});
  fake->responses.push_back({200, "{}"});

  triorb_driver::TriOrbClient client(std::move(transport));

  client.wakeup();
  client.stop();
  client.sleep();

  ASSERT_EQ(fake->requests.size(), 3);

  EXPECT_EQ(fake->requests[0].path, "/control/motor/wakeup");

  EXPECT_EQ(fake->requests[1].path, "/navigation/drive/stop");

  EXPECT_EQ(fake->requests[2].path, "/control/motor/sleep");
}

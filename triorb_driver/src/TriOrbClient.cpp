#include <triorb_driver/TriOrbClient.h>

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{

constexpr double pi = 3.14159265358979323846;

} // namespace

namespace triorb_driver
{

TriOrbClient::TriOrbClient(std::unique_ptr<HttpTransport> transport) : transport_(std::move(transport))
{
  if(!transport_)
  {
    throw std::invalid_argument("TriOrbClient requires an HTTP transport");
  }
}

void TriOrbClient::requireSuccess(const HttpResponse & response, const std::string & operation) const
{
  if(response.status >= 200 && response.status < 300)
  {
    return;
  }

  std::ostringstream message;

  message << "TriOrb " << operation << " failed with HTTP " << response.status;

  if(!response.body.empty())
  {
    message << ": " << response.body;
  }

  throw std::runtime_error(message.str());
}

void TriOrbClient::checkHealth()
{
  const auto response = transport_->get("/system/health");

  requireSuccess(response, "health check");

  const auto data = nlohmann::json::parse(response.body);

  if(data.value("status", std::string{}) != "ok")
  {
    throw std::runtime_error("TriOrb health response did not report status=ok");
  }
}

void TriOrbClient::wakeup()
{
  const auto response = transport_->post("/control/motor/wakeup", "{}");

  requireSuccess(response, "motor wakeup");
}

void TriOrbClient::sleep()
{
  const auto response = transport_->post("/control/motor/sleep", "{}");

  requireSuccess(response, "motor sleep");
}

void TriOrbClient::stop()
{
  const auto response = transport_->post("/navigation/drive/stop", "{}");

  requireSuccess(response, "drive stop");
}

nlohmann::json TriOrbClient::makeVelocityBody(double vx, double vy, double wz) const
{
  /*
   * API-specific boundary.
   *
   * Verify this object against:
   *   http://<AMR-IP>:8080/openapi.json
   *
   * Only this function and its unit test should change if
   * the installed run_vel schema differs.
   */
  return {
      {
          "velocity",
          {
              {"x", vx},
              {"y", vy},
              {"w", wz},
          },
      },
  };
}

void TriOrbClient::setBodyVelocity(double vx, double vy, double wz)
{
  const auto body = makeVelocityBody(vx, vy, wz);

  const auto response = transport_->post("/control/drive/run_vel", body.dump());

  requireSuccess(response, "body velocity command");
}

PlanarPose TriOrbClient::readPose()
{
  const auto response = transport_->get("/vslam/robot/position");

  if(response.status == 404)
  {
    return {};
  }

  requireSuccess(response, "VSLAM pose");

  const auto data = nlohmann::json::parse(response.body);

  if(!data.contains("robot_position"))
  {
    throw std::runtime_error("TriOrb pose response has no robot_position");
  }

  const auto & pose = data.at("robot_position");

  if(!pose.contains("x") || !pose.contains("y") || !pose.contains("theta_deg"))
  {
    throw std::runtime_error("TriOrb robot_position is incomplete");
  }

  PlanarPose result;

  result.x = pose.at("x").get<double>();
  result.y = pose.at("y").get<double>();

  const double clockwiseDegrees = pose.at("theta_deg").get<double>();

  result.theta = -clockwiseDegrees * pi / 180.0;

  if(!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.theta))
  {
    throw std::runtime_error("TriOrb pose contains a non-finite value");
  }

  result.valid = true;

  return result;
}

} // namespace triorb_driver

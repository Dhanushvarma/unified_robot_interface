#pragma once

#include <triorb_driver/HttpTransport.h>
#include <triorb_driver/Types.h>

#include <memory>
#include <string>

#include <nlohmann/json.hpp>

namespace triorb_driver
{

class ITriOrbClient
{
public:
  virtual ~ITriOrbClient() = default;

  virtual void checkHealth() = 0;
  virtual void wakeup() = 0;
  virtual void sleep() = 0;
  virtual void stop() = 0;

  virtual void setBodyVelocity(double vx, double vy, double wz) = 0;

  virtual PlanarPose readPose() = 0;
};

class TriOrbClient final : public ITriOrbClient
{
public:
  explicit TriOrbClient(std::unique_ptr<HttpTransport> transport);

  void checkHealth() override;
  void wakeup() override;
  void sleep() override;
  void stop() override;

  void setBodyVelocity(double vx, double vy, double wz) override;

  PlanarPose readPose() override;

private:
  nlohmann::json makeVelocityBody(double vx, double vy, double wz) const;

  void requireSuccess(const HttpResponse & response, const std::string & operation) const;

private:
  std::unique_ptr<HttpTransport> transport_;
};

} // namespace triorb_driver

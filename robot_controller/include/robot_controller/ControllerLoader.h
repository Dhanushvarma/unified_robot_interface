#pragma once

#include <robot_controller/Controller.h>

#include <memory>
#include <string>

namespace robot_controller
{

/// Loads a Controller backend plugin from `<libdir>/robot_controller/`.
class ControllerLoader
{
public:
  struct ControllerDeleter
  {
    using DestroyFunction = void (*)(Controller *);

    DestroyFunction destroy = nullptr;

    void operator()(Controller * controller) const noexcept
    {
      if(controller && destroy)
      {
        destroy(controller);
      }
    }
  };

  /// Owning pointer that destroys the controller through its plugin.
  using ControllerPtr = std::unique_ptr<Controller, ControllerDeleter>;

  ControllerLoader();
  ~ControllerLoader();

  ControllerLoader(const ControllerLoader &) = delete;
  ControllerLoader & operator=(const ControllerLoader &) = delete;

  ControllerLoader(ControllerLoader &&) = delete;
  ControllerLoader & operator=(ControllerLoader &&) = delete;

  /// Create a controller of the given backend (e.g. "mc_rtc"), passing it the
  /// configuration as a YAML string. Throws if the backend cannot be loaded.
  [[nodiscard]] ControllerPtr create(const std::string & backend, const std::string & config_data);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace robot_controller

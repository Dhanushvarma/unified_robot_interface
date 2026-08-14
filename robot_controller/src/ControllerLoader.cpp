#include <robot_controller/ControllerLoader.h>
#include <robot_controller/config.h>

#include <dlfcn.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace robot_controller
{

namespace
{

using BackendNameFunction = const char * (*)();

using CreateFunction = Controller * (*)(const char *);

using DestroyFunction = void (*)(Controller *);

std::filesystem::path pluginPath(const std::string & backend)
{
  return std::filesystem::path(ROBOT_CONTROLLER_PLUGIN_INSTALL_PREFIX) / ("librobot_controller_" + backend + ".so");
}

template<typename Function>
Function loadSymbol(void * handle, const char * symbol, const std::filesystem::path & plugin_path)
{
  dlerror();

  auto function = reinterpret_cast<Function>(dlsym(handle, symbol));

  if(const char * error = dlerror())
  {
    throw std::runtime_error("Controller plugin '" + plugin_path.string() + "' does not export '" + symbol
                             + "': " + error);
  }

  return function;
}

} // namespace

class ControllerLoader::Impl
{
public:
  struct Plugin
  {
    void * handle = nullptr;
    CreateFunction create = nullptr;
    DestroyFunction destroy = nullptr;
  };

  ~Impl()
  {
    for(auto & [backend, plugin] : plugins)
    {
      if(plugin.handle)
      {
        dlclose(plugin.handle);
      }
    }
  }

  Plugin & load(const std::string & backend)
  {
    const auto existing = plugins.find(backend);

    if(existing != plugins.end())
    {
      return existing->second;
    }

    const auto plugin_path = pluginPath(backend);

    dlerror();

    void * handle = dlopen(plugin_path.c_str(), RTLD_NOW | RTLD_LOCAL);

    if(!handle)
    {
      const char * error = dlerror();

      throw std::runtime_error("Failed to load controller backend '" + backend + "' from '" + plugin_path.string()
                               + "': " + (error ? error : "unknown dlopen error"));
    }

    try
    {
      const auto backend_name = loadSymbol<BackendNameFunction>(handle, "robot_controller_backend_name", plugin_path);

      const auto create = loadSymbol<CreateFunction>(handle, "robot_controller_create", plugin_path);

      const auto destroy = loadSymbol<DestroyFunction>(handle, "robot_controller_destroy", plugin_path);

      const char * provided_backend = backend_name();

      if(!provided_backend || backend != provided_backend)
      {
        throw std::runtime_error("Controller plugin '" + plugin_path.string() + "' provides backend '"
                                 + (provided_backend ? provided_backend : "<null>") + "', but backend '" + backend
                                 + "' was requested");
      }

      auto [it, inserted] = plugins.emplace(backend, Plugin{handle, create, destroy});

      return it->second;
    }
    catch(...)
    {
      dlclose(handle);
      throw;
    }
  }

  std::unordered_map<std::string, Plugin> plugins;
};

ControllerLoader::ControllerLoader() : impl_(std::make_unique<Impl>()) {}

ControllerLoader::~ControllerLoader() = default;

ControllerLoader::ControllerPtr ControllerLoader::create(const std::string & backend, const std::string & config_data)
{
  auto & plugin = impl_->load(backend);

  Controller * controller = plugin.create(config_data.c_str());

  if(!controller)
  {
    throw std::runtime_error("Controller backend '" + backend + "' returned a null controller");
  }

  return ControllerPtr(controller, ControllerDeleter{plugin.destroy});
}

} // namespace robot_controller

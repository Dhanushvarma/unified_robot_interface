#pragma once

#include <mc_rtc/loader.h>
#include <mc_rtc/logging.h>

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc_robot_interface
{

/// ABI check applied to every plugin library: `symbol` names an exported
/// `unsigned int ()` function whose result must equal `version`.
struct PluginAbi
{
  /// Name of the exported ABI version function.
  std::string symbol;
  /// Expected ABI version.
  unsigned int version;
};

/// Generic plugin loader wrapping mc_rtc::ObjectLoader<T>.
///
/// \param symbol The C discovery symbol exported by each plugin shared library
///        (e.g. "MC_RTC_ROBOT_DRIVER").
/// \param paths Directories to search for plugin libraries, typically set at
///        compile time via MC_ROBOT_INTERFACE_INSTALL_PREFIX (config.h).
/// \param abi If set, plugins whose ABI version differs (or is missing) are
///        refused by load() instead of being created.
template<typename T>
class PluginLoader
{
public:
  PluginLoader(const std::string & symbol,
               std::vector<std::string> paths,
               bool verbose = false,
               std::optional<PluginAbi> abi = std::nullopt)
  : symbol_(symbol), paths_(std::move(paths)), verbose_(verbose), abi_(std::move(abi))
  {
  }

  /// Create the plugin registered as `name`, forwarding `args` to its create().
  /// Throws mc_rtc::LoaderException if it is not found or has the wrong ABI.
  template<typename... Args>
  std::shared_ptr<T> load(const std::string & name, const Args &... args)
  {
    std::unique_lock<std::recursive_mutex> guard(mtx_);
    init();

    if(!loader_->has_object(name))
    {
      mc_rtc::log::error("[PluginLoader] Plugin '{}' not found", name);
      mc_rtc::log::info("[PluginLoader] Searched paths:");
      for(const auto & p : paths_)
      {
        mc_rtc::log::info("  - {}", p);
      }
      mc_rtc::log::info("[PluginLoader] Available plugins:");
      for(const auto & p : available())
      {
        mc_rtc::log::info("  - {}", p);
      }
      mc_rtc::log::error_and_throw<mc_rtc::LoaderException>("[PluginLoader] Cannot load plugin: {}", name);
    }

    if(auto it = abi_mismatches_.find(name); it != abi_mismatches_.end())
    {
      const auto & [path, found] = it->second;
      mc_rtc::log::error_and_throw<mc_rtc::LoaderException>(
          "[PluginLoader] Plugin '{}' ({}) was built against an incompatible interface: {} (expected ABI version {}). "
          "Rebuild and reinstall it against the current headers.",
          name, path, found ? fmt::format("ABI version {}", *found) : std::string{"no ABI version"}, abi_->version);
    }

    auto plugin = loader_->create_object(name, args...);
    if(!plugin)
    {
      mc_rtc::log::error_and_throw("[PluginLoader] Failed to instantiate plugin: {}", name);
    }

    return plugin;
  }

  /// Names of all plugins found in the search paths.
  std::vector<std::string> available()
  {
    std::unique_lock<std::recursive_mutex> guard(mtx_);
    init();
    return loader_->objects();
  }

private:
  void init()
  {
    if(loader_) return;
    loader_ =
        std::make_unique<mc_rtc::ObjectLoader<T>>(symbol_, paths_, verbose_,
                                                  [this](const std::string & class_name, mc_rtc::LTDLHandle & handle)
                                                  {
                                                    if(mc_rtc::Loader::default_cb)
                                                      mc_rtc::Loader::default_cb(class_name, handle);
                                                    if(!abi_) return;
                                                    auto abi_fn =
                                                        handle.template get_symbol<unsigned int (*)()>(abi_->symbol);
                                                    std::optional<unsigned int> found;
                                                    if(abi_fn) found = abi_fn();
                                                    if(found != abi_->version)
                                                      abi_mismatches_[class_name] = {handle.path(), found};
                                                  });
  }

  std::string symbol_;
  std::vector<std::string> paths_;
  bool verbose_;
  std::optional<PluginAbi> abi_;
  // Plugin name -> (library path, ABI version found, if any).
  std::map<std::string, std::pair<std::string, std::optional<unsigned int>>> abi_mismatches_;
  std::unique_ptr<mc_rtc::ObjectLoader<T>> loader_;
  std::recursive_mutex mtx_;
};

} // namespace mc_robot_interface

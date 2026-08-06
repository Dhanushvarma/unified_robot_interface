#pragma once

#include <mc_rtc/loader.h>
#include <mc_rtc/logging.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace mc_robot_interface
{

// Generic plugin loader wrapping mc_rtc::ObjectLoader<T>.
// symbol: the C discovery symbol exported by each plugin shared library
//         (e.g. "MC_RTC_ROBOT_DRIVER").
// paths:  directories to search for plugin libraries, typically set at
//         compile time via MC_ROBOT_INTERFACE_INSTALL_PREFIX (config.h).
template<typename T>
class PluginLoader
{
public:
  PluginLoader(const std::string & symbol, std::vector<std::string> paths, bool verbose = false)
  : symbol_(symbol), paths_(std::move(paths)), verbose_(verbose)
  {
  }

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

    auto plugin = loader_->create_object(name, args...);
    if(!plugin)
    {
      mc_rtc::log::error_and_throw("[PluginLoader] Failed to instantiate plugin: {}", name);
    }

    return plugin;
  }

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
    loader_ = std::make_unique<mc_rtc::ObjectLoader<T>>(symbol_, paths_, verbose_);
  }

  std::string symbol_;
  std::vector<std::string> paths_;
  bool verbose_;
  std::unique_ptr<mc_rtc::ObjectLoader<T>> loader_;
  std::recursive_mutex mtx_;
};

} // namespace mc_robot_interface

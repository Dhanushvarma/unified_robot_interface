# Configures a robot_interface driver plugin target so its installed .so
# carries RUNPATH entries for its own direct link-time dependencies
# (rclcpp, mc_rtc, the driver's SDK, ...). Without this, dlopen()-ing the
# plugin from mc_rtc's ObjectLoader requires every one of those dependency
# directories to also be listed in a system-wide ld.so.conf.d entry (or the
# loading process to have them on LD_LIBRARY_PATH already).
#
# Usage, after add_library(<target> SHARED ...):
#   add_robot_driver(<target>)
macro(add_robot_driver target)
  set_target_properties(${target} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endmacro()

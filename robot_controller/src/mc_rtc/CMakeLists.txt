include(GNUInstallDirs)

jrl_find_package(mc_rtc REQUIRED)

add_library(robot_controller_mc_rtc SHARED ControllerMcRtc.cpp)

add_library(robot_controller::mc_rtc ALIAS robot_controller_mc_rtc)

set_target_properties(
  robot_controller_mc_rtc PROPERTIES OUTPUT_NAME robot_controller_mc_rtc
                                     EXPORT_NAME mc_rtc
)

target_include_directories(
  robot_controller_mc_rtc
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(
  robot_controller_mc_rtc
  PUBLIC robot_controller::core
  PRIVATE mc_rtc::mc_control
)

install(
  TARGETS robot_controller_mc_rtc
  EXPORT robot_controllerTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/robot_controller
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/robot_controller
)

install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/robot_controller/mc_rtc/ControllerMcRtc.h"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/robot_controller/mc_rtc
)

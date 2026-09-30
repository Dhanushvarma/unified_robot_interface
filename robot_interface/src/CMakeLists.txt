set(MC_ROBOT_INTERFACE_INSTALL_PREFIX
    "${CMAKE_INSTALL_PREFIX}/lib/robot_interface"
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/AddRobotDriver.cmake)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/robot_interface/config.in.h
  ${CMAKE_CURRENT_BINARY_DIR}/../include/robot_interface/config.h @ONLY
)

# -----------------------------------------------------------------------
# robot_driver_api: header-only target for driver plugin authors.
# Provides RobotDriverTemplate.h and driver/api.h with no mc_rtc dep,
# so that rtde_driver, mc_dummy, etc. can find_package(mc_robot_interface)
# without pulling in mc_rtc and its ROS2 transitive deps.
# -----------------------------------------------------------------------
add_library(robot_driver_api INTERFACE)
target_include_directories(
  robot_driver_api
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
            $<INSTALL_INTERFACE:include>
)

# -----------------------------------------------------------------------
# robot_driver_loader: plugin loader API and RobotDriver interface
# mc_rtc is a PRIVATE dep — it is an implementation detail of the loader
# and must not be propagated to driver plugins.
# -----------------------------------------------------------------------
add_library(robot_driver_loader SHARED driver/RobotDriverLoader.cpp)
target_link_libraries(robot_driver_loader PRIVATE mc_rtc::mc_rtc_loader)
target_link_libraries(robot_driver_loader PUBLIC robot_driver_api)
target_include_directories(
  robot_driver_loader
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

# -----------------------------------------------------------------------
# mc_robot_interface_lib: robot-side always-running process library
# -----------------------------------------------------------------------
find_package(robot_comm QUIET)

add_library(
  mc_robot_interface_lib SHARED RobotInterface.cpp RobotStateViewer.cpp
)
target_link_libraries(
  mc_robot_interface_lib PUBLIC robot_driver_loader mc_rtc::mc_rtc_utils
)

if(robot_comm_FOUND)
  target_link_libraries(mc_robot_interface_lib PUBLIC robot_comm::robot_comm)
else()
  # Monorepo build: targets are defined by robot_comm subdirectory.
  target_link_libraries(mc_robot_interface_lib PUBLIC robot_comm zenoh)
endif()

target_include_directories(
  mc_robot_interface_lib
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

# robot_driver_api and robot_driver_loader are exported so that driver packages
# (rtde_driver, mc_dummy, etc.) can find_package(mc_robot_interface).
install(TARGETS robot_driver_api EXPORT mc_robot_interfaceTargets)

install(
  TARGETS robot_driver_loader
  EXPORT mc_robot_interfaceTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

# mc_robot_interface_lib is an internal implementation detail (depends on
# robot_comm which has its own install lifecycle), so it is not exported.
install(
  TARGETS mc_robot_interface_lib
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

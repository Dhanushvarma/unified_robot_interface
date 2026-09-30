cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME unified_robot_interface)
set(PROJECT_VERSION 1.0.0)
project(
  ${PROJECT_NAME}
  LANGUAGES CXX
  VERSION ${PROJECT_VERSION}
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_subdirectory(robot_comm)
add_subdirectory(robot_controller)
add_subdirectory(robot_interface)

add_subdirectory(robot_manager)

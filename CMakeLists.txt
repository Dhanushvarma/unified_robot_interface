cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME robot_manager_project)
set(PROJECT_VERSION 1.0.0)
project(
  ${PROJECT_NAME}
  LANGUAGES CXX
  VERSION ${PROJECT_VERSION}
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(robot_comm)
add_subdirectory(robot_interface)

add_subdirectory(robot_manager)

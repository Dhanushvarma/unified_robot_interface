cmake_minimum_required(VERSION 3.14)

# Project Name
set(PROJECT_NAME robot_manager_project)
set(PROJECT_VERSION 1.0.0)
project(
  ${PROJECT_NAME}
  LANGUAGES CXX
  VERSION ${PROJECT_VERSION}
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(mc_communication)
add_subdirectory(robot_interface)

# add_subdirectory(robot_manager)
# add_subdirectory(local_robot)

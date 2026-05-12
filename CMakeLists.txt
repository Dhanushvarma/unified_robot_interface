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

find_package(zenoh-c QUIET)
if(NOT zenoh-c_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    zenoh-c
    GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-c.git
    GIT_TAG main
  )
  FetchContent_MakeAvailable(zenoh-c)
endif()

find_package(zenoh-cpp QUIET)
if(NOT zenoh-cpp_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    zenoh-cpp
    GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-cpp.git
    GIT_TAG main
  )
  FetchContent_MakeAvailable(zenoh-cpp)
endif()

add_subdirectory(robot_manager)
add_subdirectory(local_robot)

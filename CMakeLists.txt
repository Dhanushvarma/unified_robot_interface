cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME mc_robot_interface)
set(PROJECT_VERSION 1.0.0)
set(CXX_DISABLE_WERROR 1)
set(CMAKE_CXX_STANDARD 17)

project(
  ${PROJECT_NAME}
  LANGUAGES CXX
  VERSION ${PROJECT_VERSION})

if(NOT TARGET mc_rtc::mc_control)
  find_package(mc_rtc REQUIRED)
endif()

add_subdirectory(src)

if(${BUILD_TESTING})
  add_subdirectory(tests)
endif()

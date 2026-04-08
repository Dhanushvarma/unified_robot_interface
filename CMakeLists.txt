cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME mc_robot_interface)
set(PROJECT_VERSION 1.0.0)
set(CXX_DISABLE_WERROR 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
  ${PROJECT_NAME}
  LANGUAGES CXX
  VERSION ${PROJECT_VERSION}
)

option(BUILD_TESTING "Build tests" OFF)

if(NOT TARGET mc_rtc::mc_control)
  find_package(mc_rtc REQUIRED)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
add_subdirectory(src)

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

# install(FILES etc/mc_rtc.yaml DESTINATION etc)

message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests: ${BUILD_TESTING}")
message(STATUS "")

cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME rtde_driver)
set(PROJECT_VERSION 1.0.0)
project(
  ${PROJECT_NAME}
  LANGUAGES CXX
  VERSION ${PROJECT_VERSION}
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CTest)
include(FetchContent)

# enable_testing()

option(BUILD_TESTING "Build tests" OFF)

# Dependencies
find_package(mc_robot_interface REQUIRED)
find_package(fmt REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json QUIET)

if(NOT nlohmann_json_FOUND)
  message(STATUS "nlohmann_json not found, fetching it")

  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
  )

  FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(httplib QUIET)

if(NOT httplib_FOUND)
  message(STATUS "cpp-httplib not found, fetching it")

  FetchContent_Declare(
    cpp_httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.3
    GIT_SHALLOW TRUE
  )

  FetchContent_MakeAvailable(cpp_httplib)
endif()

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

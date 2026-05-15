cmake_minimum_required(VERSION 3.14)

# Project Name
set(PROJECT_NAME mc_local)
set(PROJECT_VERSION 1.0.0)
project(
  ${PROJECT_NAME}
  LANGUAGES CXX
  VERSION ${PROJECT_VERSION}
)

set(CXX_DISABLE_WERROR 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# RPATH settings
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Dependencies
find_package(mc_rtc REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options thread)
find_program(SETCAP setcap)
find_program(SUDO sudo)

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

# Add Subdirectories
add_subdirectory(src)

# Install the executable to the system binary directory
install(TARGETS mc_local RUNTIME DESTINATION bin)

install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})

message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests: ${BUILD_TESTING}")
message(STATUS "  Install RPATH: ${CMAKE_INSTALL_RPATH}")
message(STATUS "")

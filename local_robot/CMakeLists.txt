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
find_package(mc_fleet REQUIRED)
find_package(mc_rtc REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options thread)
# find_package(Threads REQUIRED)

# Add Subdirectories
include_directories(include)
add_subdirectory(src)

install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})

# # Optional Testing
# option(BUILD_TESTING "Build tests" OFF)
# if(BUILD_TESTING)
#   enable_testing()
# endif()

message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests: ${BUILD_TESTING}")
message(STATUS "  Install RPATH: ${CMAKE_INSTALL_RPATH}")
message(STATUS "")

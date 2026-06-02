cmake_minimum_required(VERSION 3.22)

# Project Name
set(PROJECT_NAME mc_fleet)
set(PROJECT_VERSION 1.0.0)
project(
  ${PROJECT_NAME}
  LANGUAGES CXX
  VERSION ${PROJECT_VERSION}
)

set(CXX_DISABLE_WERROR 1)
set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
find_package(Boost REQUIRED COMPONENTS program_options thread)
find_package(mc_rtc REQUIRED)
find_package(Threads REQUIRED)
find_program(SETCAP setcap)
find_program(SUDO sudo)
if(NOT SUDO)
  message(FATAL_ERROR "sudo must be available on the system")
else()
  message("-- sudo command: ${SUDO}")
endif()

# Add Subdirectories
add_subdirectory(src)

# Optional Testing
option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
  add_subdirectory(tests)
  enable_testing()
endif()

install(DIRECTORY include/ DESTINATION include)

message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests: ${BUILD_TESTING}")
message(STATUS "  Install RPATH: ${CMAKE_INSTALL_RPATH}")
message(STATUS "")

# Export
include(CMakePackageConfigHelpers)

set(TARGETS_EXPORT_NAME "mc_fleetTargets")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/mc_fleetConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/mc_fleetConfig.cmake"
  INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/mc_fleetConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/mc_fleetConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/mc_fleetConfigVersion.cmake"
  DESTINATION lib/cmake/${PROJECT_NAME}
)

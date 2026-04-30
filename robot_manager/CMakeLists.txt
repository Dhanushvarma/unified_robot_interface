cmake_minimum_required(VERSION 3.14)

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
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# RPATH settings
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Dependencies
find_package(Boost REQUIRED COMPONENTS program_options thread)
find_package(Flatbuffers REQUIRED)
find_package(mc_rtc REQUIRED)
find_package(Threads REQUIRED)
find_package(zenohc REQUIRED)
find_package(zenohcxx REQUIRED)

# Flatbuffers
find_program(FLATC_EXECUTABLE NAMES flatc REQUIRED)
set(SCHEMA_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mc_communication/flatbuffers/Message.fbs"
)
set(OUTPUT_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mc_communication/flatbuffers"
)
add_custom_command(
  OUTPUT "${OUTPUT_DIR}/Message_generated.h"
  COMMAND ${FLATC_EXECUTABLE} --cpp -o "${OUTPUT_DIR}" "${SCHEMA_FILE}"
  DEPENDS "${SCHEMA_FILE}"
  VERBATIM
)
add_custom_target(generate_fbs DEPENDS "${OUTPUT_DIR}/Message_generated.h")

# Add Subdirectories
include_directories(include)
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

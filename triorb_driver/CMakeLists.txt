cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME triorb_driver)
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

# enable_testing()
option(BUILD_CONTROLLER "Build testing controller for Triorb and Call-M" OFF)

# Dependencies
find_package(mc_robot_interface REQUIRED)
find_package(fmt REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(src)

# if(BUILD_TESTING)
#   add_subdirectory(tests)
# endif()

if(BUILD_CONTROLLER)
  message(STATUS "BUILD_CONTROLLER is enabled!")
  add_subdirectory(test_controllers/triorb_controller)
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

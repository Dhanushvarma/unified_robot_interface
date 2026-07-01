cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME mc_robot_interface)
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

enable_testing()

option(BUILD_TESTING "Build tests" OFF)

# Dependencies
# find_package(mc_communication REQUIRED) # TODO uncomment when install will be setup in mc_communication
find_package(mc_rtc REQUIRED)

# Add Subdirectories
add_subdirectory(src)

# Optional Testing
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(DIRECTORY include/ DESTINATION include)

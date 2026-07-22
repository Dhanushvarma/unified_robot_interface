cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME mc_robot_interface)
set(PROJECT_VERSION 1.0.0)
project(
  ${PROJECT_NAME}
  LANGUAGES CXX
  VERSION ${PROJECT_VERSION}
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
find_package(mc_rtc REQUIRED)

# Add source subdirectory
add_subdirectory(src)

# Install public headers
install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h")

cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME rtde_driver)
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

# enable_testing()

option(BUILD_TESTING "Build tests" OFF)

# Dependencies
find_package(mc_robot_interface REQUIRED)
find_package(ur_client_library REQUIRED)
find_package(fmt REQUIRED)

# Add Subdirectories
add_subdirectory(src)

# # Optional Testing
# if(BUILD_TESTING)
#   add_subdirectory(tests)
# endif()

install(DIRECTORY include/ DESTINATION include)

cmake_minimum_required(VERSION 3.14)
find_package(GTest CONFIG)

if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE
  )
  FetchContent_MakeAvailable(googletest)
endif()
include(GoogleTest)

# ── E2E tests ──
add_executable(test_config_delivery test_config_delivery.cpp)
target_link_libraries(test_config_delivery PRIVATE GTest::gtest_main)

# Register with special label so it can be opted in/out
gtest_discover_tests(
  test_config_delivery
  PROPERTIES LABELS "e2e"
  TIMEOUT 30
)

add_executable(test_local_launch test_local_launch.cpp)
target_link_libraries(test_local_launch PRIVATE GTest::gtest_main)
gtest_discover_tests(
  test_local_launch
  PROPERTIES LABELS "launch"
  TIMEOUT 30
)

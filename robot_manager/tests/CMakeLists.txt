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

add_executable(test_serialization test_serialization.cpp)
target_link_libraries(
  test_serialization PRIVATE mc_communication GTest::gtest_main
)
gtest_discover_tests(test_serialization)

# add_executable(test_e2e test_e2e.cpp)
# target_link_libraries(test_e2e PRIVATE GTest::gtest_main)
# gtest_discover_tests(test_e2e)

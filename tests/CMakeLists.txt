find_package(GTest)
if(NOT GTest_FOUND)
  include(FetchContent)
  fetchcontent_declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE
  )
  fetchcontent_makeavailable(googletest)
endif()

add_executable(test_init test_init.cpp)

target_link_libraries(
  test_init PRIVATE mc_robot_manager mc_robot_interface mc_network_interface
                    mc_rtc::mc_control gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_init)

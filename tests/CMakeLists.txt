find_package(GTest REQUIRED)

add_executable(test_init test_init.cpp)

target_link_libraries(test_init PRIVATE mc_robot_interface GTest::gtest_main)

include(GoogleTest)

gtest_discover_tests(test_init)

find_package(GTest REQUIRED)

add_executable(test_TriOrbClient test_TriOrbClient.cpp)

target_link_libraries(test_TriOrbClient PRIVATE TriOrbClient GTest::gtest_main)

add_test(NAME test_TriOrbClient COMMAND test_TriOrbClient)

add_executable(test_RobotDriverTriOrb test_RobotDriverTriOrb.cpp)

target_link_libraries(
  test_RobotDriverTriOrb PRIVATE RobotDriverTriOrbCore GTest::gtest_main
)

add_test(NAME test_RobotDriverTriOrb COMMAND test_RobotDriverTriOrb)

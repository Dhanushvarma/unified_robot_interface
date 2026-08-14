find_package(GTest REQUIRED)
include(GoogleTest)

function(add_test TEST_NAME)
  add_executable(${TEST_NAME} ${ARGN})
  target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest_main)

  gtest_discover_tests(${TEST_NAME} LABELS "controller" TIMEOUT 10)
endfunction()

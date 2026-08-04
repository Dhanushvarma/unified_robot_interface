# -----------------------------------------------------------------------
# robot_state_viewer: standalone utility to display live robot state.
# Loads a driver plugin directly — no communication stack needed.
# -----------------------------------------------------------------------
add_executable(robot_state_viewer robot_state_viewer.cpp)
target_include_directories(
  robot_state_viewer PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../third_party
)
target_link_libraries(
  robot_state_viewer PRIVATE robot_driver_loader mc_rtc::mc_control
)

install(TARGETS robot_state_viewer DESTINATION bin)

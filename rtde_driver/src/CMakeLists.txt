add_library(RobotDriverRTDE SHARED RobotDriverRTDE.cpp)

target_include_directories(
  RobotDriverRTDE
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(
  RobotDriverRTDE PUBLIC mc_robot_interface::robot_driver_api
)
target_link_libraries(RobotDriverRTDE PRIVATE ur_client_library::urcl fmt::fmt)

install(TARGETS RobotDriverRTDE LIBRARY DESTINATION lib/robot_interface)

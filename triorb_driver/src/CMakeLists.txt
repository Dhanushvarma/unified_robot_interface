add_library(RobotDriverTriOrb SHARED Plugin.cpp RobotDriverTriOrb.cpp)

target_include_directories(
  RobotDriverTriOrb
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(
  RobotDriverTriOrb PUBLIC mc_robot_interface::robot_driver_api
)

target_link_libraries(RobotDriverTriOrb PRIVATE fmt::fmt Threads::Threads)

install(
  TARGETS RobotDriverTriOrb
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/robot_interface
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/robot_interface
)

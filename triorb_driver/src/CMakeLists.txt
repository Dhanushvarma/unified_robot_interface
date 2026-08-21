add_library(TriOrbClient STATIC HttpTransport.cpp TriOrbClient.cpp)

set_target_properties(TriOrbClient PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(
  TriOrbClient PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                      $<INSTALL_INTERFACE:include>
)

target_link_libraries(
  TriOrbClient PUBLIC nlohmann_json::nlohmann_json httplib::httplib
                      Threads::Threads
)

add_library(RobotDriverTriOrbCore STATIC RobotDriverTriOrb.cpp)

set_target_properties(
  RobotDriverTriOrbCore PROPERTIES POSITION_INDEPENDENT_CODE ON
)

target_include_directories(
  RobotDriverTriOrbCore
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(
  RobotDriverTriOrbCore PUBLIC TriOrbClient
                               mc_robot_interface::robot_driver_api fmt::fmt
)

add_library(RobotDriverTriOrb SHARED Plugin.cpp)

target_link_libraries(RobotDriverTriOrb PRIVATE RobotDriverTriOrbCore)

install(
  TARGETS RobotDriverTriOrb
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/robot_interface
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/robot_interface
)

### Library
set(MC_ROBOT_INTERFACE_SRC FMInterfaceTemplate.cpp RobotInterfaceFactory.cpp)

add_library(mc_robot_interface SHARED ${MC_ROBOT_INTERFACE_SRC})
add_library(${PROJECT_NAME}::mc_robot_interface ALIAS mc_robot_interface)

target_link_libraries(
  mc_robot_interface
  PUBLIC mc_rtc::mc_control
         mc_communication::mc_communication # depends on mc_communication
)

target_include_directories(
  mc_robot_interface
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

### Install
install(
  TARGETS mc_robot_interface
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

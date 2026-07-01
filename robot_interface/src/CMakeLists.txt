set(MC_ROBOT_INTERFACE_INSTALL_PREFIX
    "${CMAKE_INSTALL_PREFIX}/lib/robot_driver"
)

macro(add_robot_driver NAME)

endmacro()

# add_library(robot_interface_template SHARED )

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/robot_interface/config.in.h
  ${CMAKE_CURRENT_BINARY_DIR}/../include/robot_interface/config.h @ONLY
)
add_library(robot_driver_loader SHARED driver/RobotDriverLoader.cpp)
target_link_libraries(robot_driver_loader PUBLIC mc_rtc::mc_rtc_loader)

target_include_directories(
  robot_driver_loader
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

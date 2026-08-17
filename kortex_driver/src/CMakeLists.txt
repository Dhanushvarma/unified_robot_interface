add_library(RobotDriverKortex SHARED RobotDriverKortex.cpp)

# target_compile_definitions(
#   RobotDriverKortex
#   PRIVATE
#     MC_ROBOT_DRIVER_EXPORTS
# )

target_include_directories(
  RobotDriverKortex
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(
  RobotDriverKortex
  PUBLIC mc_robot_interface::robot_driver_api
  PRIVATE Kortex::Api fmt::fmt
)

install(
  TARGETS RobotDriverKortex
  # RUNTIME DESTINATION bin
  # LIBRARY DESTINATION lib/robot_interface
  ARCHIVE DESTINATION lib/robot_interface
)

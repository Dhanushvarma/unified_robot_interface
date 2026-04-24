# Network Interface
set(NETWORK_INTERFACE_SRC
    mc_network_interface/NetworkInterfaceFactory.cpp
    # mc_network_interface/NetworkInterfaceShm.cpp
    mc_network_interface/NetworkInterfaceZenoh.cpp
)

add_library(mc_network_interface SHARED ${NETWORK_INTERFACE_SRC})
add_library(${PROJECT_NAME}::network_interface ALIAS mc_network_interface)

target_link_libraries(
  mc_network_interface PUBLIC mc_rtc::mc_control zenohcxx::zenohc
)

target_include_directories(
  mc_network_interface
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

# Robot Interface
set(INTERFACE_SRC mc_robot_interface/RobotInterfaceFactory.cpp
                  mc_robot_interface/InterfaceTemplate.cpp
)

add_library(mc_robot_interface SHARED ${INTERFACE_SRC})
add_library(${PROJECT_NAME}::robot_interface ALIAS mc_robot_interface)

target_link_libraries(
  mc_robot_interface PUBLIC mc_rtc::mc_control mc_network_interface
                            # mc_local::mc_local_lib
)

target_include_directories(
  mc_robot_interface
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

# Robot Manager
set(ROBOT_MANAGER_SRC mc_robot_manager/Fleet.cpp
                      mc_robot_manager/RobotManager.cpp
)

add_library(mc_robot_manager SHARED ${ROBOT_MANAGER_SRC})
add_library(${PROJECT_NAME}::robot_manager ALIAS mc_robot_manager)

target_link_libraries(
  mc_robot_manager PUBLIC mc_rtc::mc_control mc_network_interface
)

target_include_directories(
  mc_robot_manager
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

# Install all libraries
install(
  TARGETS mc_network_interface mc_robot_interface mc_robot_manager
  EXPORT "${PROJECT_NAME}Targets"
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(
  EXPORT "${PROJECT_NAME}Targets" FILE "${PROJECT_NAME}Targets.cmake" NAMESPACE
                                  ${PROJECT_NAME}::
  DESTINATION lib/cmake/${PROJECT_NAME}
)

# Executable
add_executable(MCFleetControl mc_robot_manager/main.cpp)

target_link_libraries(
  MCFleetControl PRIVATE mc_robot_manager mc_robot_interface
                         mc_network_interface boost_program_options
)

install(TARGETS MCFleetControl RUNTIME DESTINATION bin)

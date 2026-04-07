# Network Interface
set(NETWORK_INTERFACE_SRC
    mc_network_interface/TcpNetworkInterface.cpp
    mc_network_interface/UdpNetworkInterface.cpp
    mc_network_interface/ZenohNetworkInterface.cpp)

add_library(mc_network_interface INTERFACE ${NETWORK_INTERFACE_SRC})
add_library(mc_network_interface::mc_network_interface ALIAS
            mc_network_interface)
# target_link_libraries(mc_network_interface INTERFACE mc_rtc::mc_control)

target_include_directories(
  mc_network_interface INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                                 $<INSTALL_INTERFACE:include>)

# Robot Interface Template

set(INTERFACE_SRC mc_robot_interface/RobotInterfaceTemplate.cpp)

add_library(mc_robot_interface INTERFACE ${INTERFACE_SRC})
add_library(mc_rtc::mc_robot_interface ALIAS mc_robot_interface)
target_link_libraries(mc_robot_interface INTERFACE mc_rtc::mc_control
                                                   mc_network_interface)

target_include_directories(
  mc_robot_interface INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                               $<INSTALL_INTERFACE:include>)

# Robot Manager
set(ROBOT_MANAGER_SRC mc_robot_manager/RobotManager.cpp)

add_library(mc_robot_manager INTERFACE ${ROBOT_MANAGER_SRC})
add_library(mc_robot_manager::mc_robot_manager ALIAS mc_robot_manager)
target_link_libraries(mc_robot_manager INTERFACE mc_rtc::mc_control
                                                 mc_network_interface)

target_include_directories(
  mc_robot_manager INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                             $<INSTALL_INTERFACE:include>)

install(
  TARGETS mc_robot_interface mc_network_interface
  EXPORT "${TARGETS_EXPORT_NAME}"
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)

# Network Interface
set(NETWORK_INTERFACE_SRC
    mc_network_interface/TcpNetworkInterface.cpp
    mc_network_interface/UdpNetworkInterface.cpp
    mc_network_interface/ZenohNetworkInterface.cpp
)

add_library(mc_network_interface SHARED ${NETWORK_INTERFACE_SRC})
add_library(
  mc_network_interface::mc_network_interface ALIAS mc_network_interface
)

target_include_directories(
  mc_network_interface PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              $<INSTALL_INTERFACE:include>
)

# Robot Interface
set(INTERFACE_SRC mc_robot_interface/RobotInterfaceFactory.cpp
                  mc_robot_interface/RobotInterfaceUR.cpp
)

add_library(mc_robot_interface SHARED ${INTERFACE_SRC})
add_library(mc_rtc::mc_robot_interface ALIAS mc_robot_interface)

target_link_libraries(
  mc_robot_interface PUBLIC mc_rtc::mc_control mc_network_interface
)

target_include_directories(
  mc_robot_interface
  PUBLIC # ← Changed from INTERFACE
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

# Robot Manager
set(ROBOT_MANAGER_SRC mc_robot_manager/RobotManager.cpp)

add_library(mc_robot_manager SHARED ${ROBOT_MANAGER_SRC})
add_library(mc_fleet::mc_robot_manager ALIAS mc_robot_manager)

target_link_libraries(
  mc_robot_manager PUBLIC mc_rtc::mc_control mc_network_interface
)

target_include_directories(
  mc_robot_manager
  PUBLIC # ← Changed from INTERFACE
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

# Install ALL libraries (added mc_robot_manager) ← THIS WAS MISSING!
install(
  TARGETS mc_robot_interface mc_network_interface mc_robot_manager
  EXPORT "${TARGETS_EXPORT_NAME}"
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)

# Executable
add_executable(MCFleetControl mc_robot_manager/main.cpp)

target_link_libraries(
  MCFleetControl PRIVATE mc_robot_manager mc_robot_interface
                         mc_network_interface boost_program_options
)

install(TARGETS MCFleetControl RUNTIME DESTINATION bin)

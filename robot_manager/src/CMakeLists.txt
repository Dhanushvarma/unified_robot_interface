set(PROGRAM_NAME MCFleetControl)

# Communication Interface
set(COMMUNICATION_SRC
    mc_communication/Communication.cpp
    mc_communication/CommunicationFactory.cpp
    mc_communication/CommunicationZenoh.cpp
)

add_library(mc_communication SHARED ${COMMUNICATION_SRC})
add_library(${PROJECT_NAME}::communication ALIAS mc_communication)

add_dependencies(mc_communication generate_fbs)

target_link_libraries(
  mc_communication
  PUBLIC flatbuffers mc_rtc::mc_control
  PRIVATE zenohcxx::zenohc
)

target_include_directories(
  mc_communication
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

# Robot Interface
set(INTERFACE_SRC mc_robot_interface/RobotInterfaceFactory.cpp
                  mc_robot_interface/FMInterfaceTemplate.cpp
)

add_library(mc_robot_interface SHARED ${INTERFACE_SRC})
add_library(${PROJECT_NAME}::robot_interface ALIAS mc_robot_interface)

target_link_libraries(
  mc_robot_interface PUBLIC mc_rtc::mc_control mc_communication
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
  mc_robot_manager PUBLIC mc_rtc::mc_control mc_communication
)

target_include_directories(
  mc_robot_manager
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

# Install all libraries
install(
  TARGETS mc_communication mc_robot_interface mc_robot_manager
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
add_executable(${PROGRAM_NAME} mc_robot_manager/main.cpp)

target_link_libraries(
  ${PROGRAM_NAME} PRIVATE mc_robot_manager mc_robot_interface mc_communication
                          Boost::program_options
)

set(JOB_POOL_OPTION)
if(${CMAKE_VERSION} VERSION_GREATER "3.14")
  set(JOB_POOL_OPTION JOB_POOL console)
endif()
add_custom_target(
  setcap_${PROGRAM_NAME} ALL
  COMMAND ${SUDO} -S ${SETCAP} cap_sys_nice+eip $<TARGET_FILE:${PROGRAM_NAME}>
  DEPENDS ${PROGRAM_NAME} ${JOB_POOL_OPTION}
)

install(TARGETS ${PROGRAM_NAME} DESTINATION bin)
install(
  CODE
  "execute_process(COMMAND ${SUDO} -S ${SETCAP} cap_sys_nice+eip ${CMAKE_INSTALL_PREFIX}/bin/${PROGRAM_NAME})"
)

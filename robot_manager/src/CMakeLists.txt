# Robot Interface
set(INTERFACE_SRC mc_robot_interface/RobotInterfaceFactory.cpp
                  mc_robot_interface/FMInterfaceTemplate.cpp
)

add_library(mc_robot_interface SHARED ${INTERFACE_SRC})
add_library(${PROJECT_NAME}::robot_interface ALIAS mc_robot_interface)

target_link_libraries(mc_robot_interface PUBLIC mc_rtc::mc_control robot_comm)

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

target_link_libraries(mc_robot_manager PUBLIC mc_rtc::mc_control robot_comm)

target_include_directories(
  mc_robot_manager
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

# Install libraries (robot_comm_serializer and robot_comm are installed by their own subdirectory)
install(
  TARGETS mc_robot_interface mc_robot_manager
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

# Executable
add_executable(MCFleetControl mc_robot_manager/main.cpp)

target_link_libraries(
  MCFleetControl PRIVATE mc_robot_manager mc_robot_interface robot_comm
                         Boost::program_options
)

set(PROGRAM_NAME MCFleetControl)

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

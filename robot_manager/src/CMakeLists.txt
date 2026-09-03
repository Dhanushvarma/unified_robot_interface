include(GNUInstallDirs)

# Robot Interface
set(INTERFACE_SRC mc_robot_interface/RobotInterfaceFactory.cpp
                  mc_robot_interface/FMInterfaceTemplate.cpp
)

add_library(mc_robot_interface SHARED ${INTERFACE_SRC})
add_library(${PROJECT_NAME}::robot_interface ALIAS mc_robot_interface)

target_link_libraries(
  mc_robot_interface PUBLIC robot_controller::core robot_comm
)

target_include_directories(
  mc_robot_interface
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Robot Manager
set(ROBOT_MANAGER_SRC mc_robot_manager/Fleet.cpp
                      mc_robot_manager/RobotManager.cpp
)

add_library(mc_robot_manager SHARED ${ROBOT_MANAGER_SRC})
add_library(${PROJECT_NAME}::robot_manager ALIAS mc_robot_manager)

target_link_libraries(mc_robot_manager PUBLIC robot_controller::core robot_comm)

target_include_directories(
  mc_robot_manager
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../third_party
)

install(
  TARGETS mc_robot_interface mc_robot_manager
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Executable
add_executable(MCFleetControl mc_robot_manager/main.cpp)

target_include_directories(
  MCFleetControl PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../third_party
)
target_link_libraries(
  MCFleetControl PRIVATE mc_robot_manager mc_robot_interface robot_comm
)

# Installed executable and libraries use the same custom installation prefix.
# Use an absolute RPATH because MCFleetControl receives file capabilities.
set_target_properties(
  mc_robot_interface mc_robot_manager MCFleetControl
  PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
)

set(PROGRAM_NAME MCFleetControl)

set(JOB_POOL_OPTION)
if(CMAKE_VERSION VERSION_GREATER "3.14")
  set(JOB_POOL_OPTION JOB_POOL console)
endif()

add_custom_target(
  setcap_${PROGRAM_NAME} ALL
  COMMAND ${SUDO} -S ${SETCAP} cap_sys_nice+eip $<TARGET_FILE:${PROGRAM_NAME}>
  DEPENDS ${PROGRAM_NAME} ${JOB_POOL_OPTION}
)

install(TARGETS ${PROGRAM_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
  CODE
  "execute_process(
       COMMAND
         ${SUDO} -S ${SETCAP} cap_sys_nice+eip
         ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${PROGRAM_NAME}
     )"
)

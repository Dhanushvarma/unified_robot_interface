set(PROGRAM_NAME MCFleetControl)

set(ROBOT_MANAGER_SRC Fleet.cpp RobotManager.cpp)

add_library(mc_robot_manager SHARED ${ROBOT_MANAGER_SRC})
add_library(${PROJECT_NAME}::robot_manager ALIAS mc_robot_manager)

target_link_libraries(mc_robot_manager PUBLIC mc_rtc::mc_control)

target_include_directories(
  mc_robot_manager
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

install(
  TARGETS mc_robot_manager
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
add_executable(${PROGRAM_NAME} main.cpp)

target_link_libraries(
  ${PROGRAM_NAME} PRIVATE mc_robot_manager Boost::program_options
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

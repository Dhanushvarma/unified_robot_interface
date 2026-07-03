set(PROGRAM_NAME mc_local)

add_executable(${PROGRAM_NAME} InterfaceTemplate.cpp main.cpp)

target_include_directories(
  ${PROGRAM_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(
  ${PROGRAM_NAME} PRIVATE CMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
                          PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(
  ${PROGRAM_NAME} PRIVATE Boost::program_options
                          mc_robot_interface::mc_robot_interface
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

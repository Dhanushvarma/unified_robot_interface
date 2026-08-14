include(GNUInstallDirs)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../config.in.h"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/robot_controller/config.h" @ONLY
)

add_library(robot_controller_core SHARED ControllerLoader.cpp)

add_library(robot_controller::core ALIAS robot_controller_core)

set_target_properties(
  robot_controller_core PROPERTIES OUTPUT_NAME robot_controller_core EXPORT_NAME
                                                                     core
)

target_include_directories(
  robot_controller_core
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
  PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(robot_controller_core PRIVATE ${CMAKE_DL_LIBS})

install(
  TARGETS robot_controller_core
  EXPORT robot_controllerTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/../include/robot_controller/Controller.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../include/robot_controller/ControllerLoader.h"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/robot_controller
)

if(ROBOT_CONTROLLER_WITH_MC_RTC)
  add_subdirectory(mc_rtc)
endif()

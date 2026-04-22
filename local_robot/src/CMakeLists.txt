# src/CMakeLists.txt

set(LIB_NAME mc_local_lib)
set(EXE_NAME mc_local)

add_library(${LIB_NAME} SHARED InterfaceTemplate.cpp)
add_executable(${EXE_NAME} main.cpp)

target_link_libraries(${EXE_NAME} PRIVATE ${LIB_NAME})

target_include_directories(
  ${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                     $<INSTALL_INTERFACE:include>
)

target_link_libraries(
  ${LIB_NAME}
  PUBLIC mc_rtc::mc_control Boost::program_options
  PRIVATE mc_fleet::mc_robot_interface mc_fleet::mc_network_interface
          mc_fleet::mc_robot_manager
)

# INSTALLATION RULES
install(
  TARGETS ${LIB_NAME}
  EXPORT mc_localTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(TARGETS ${EXE_NAME} DESTINATION bin)

set(EXE_NAME mc_local)

add_executable(${EXE_NAME} InterfaceTemplate.cpp main.cpp)

target_include_directories(
  ${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(
  ${EXE_NAME} PRIVATE Boost::program_options mc_fleet::robot_interface
)

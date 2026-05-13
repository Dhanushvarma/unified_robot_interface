set(EXE_NAME mc_local)

add_executable(${EXE_NAME} main.cpp)

target_link_libraries(
  ${EXE_NAME} PRIVATE Boost::program_options mc_fleet::robot_interface
)

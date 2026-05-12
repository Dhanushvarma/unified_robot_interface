set(EXE_NAME mc_local)

add_executable(${EXE_NAME} main.cpp)

target_link_libraries(
  ${EXE_NAME}
  PRIVATE Boost::program_options
          mc_robot_interface # Use the raw target name, not mc_fleet::
          # mc_communication # You likely need this too
)

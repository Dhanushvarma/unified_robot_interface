cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME robot_comm)
set(PROJECT_VERSION 1.0.0)
project(
  ${PROJECT_NAME}
  LANGUAGES CXX
  VERSION ${PROJECT_VERSION}
)

# ── jrl-cmakemodules v2 ───────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
  JRL_CMAKE_MODULES
  GIT_REPOSITORY https://github.com/jrl-umi3218/jrl-cmakemodules.git
  GIT_TAG master
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JRL_CMAKE_MODULES)
include("${jrl_cmake_modules_SOURCE_DIR}/v2/modules/jrl.cmake")

jrl_configure_defaults()
set(CMAKE_CXX_STANDARD 17)

# ── Options ───────────────────────────────────────────────────────────────────
jrl_option(BUILD_TESTING "Build tests" ON)
jrl_option(WITH_PROTOBUF "Build Protobuf message" ON)

# ── Dependencies ──────────────────────────────────────────────────────────────
jrl_find_package(FlatBuffers REQUIRED)
jrl_find_package(mc_rtc REQUIRED) # TODO: remove — used only for logging
jrl_find_package(zenohc REQUIRED)
jrl_find_package(zenohcxx REQUIRED)
jrl_find_package(protobuf QUIET)

# ── Flatbuffers code generation ───────────────────────────────────────────────
flatbuffers_generate_headers(
  TARGET
  mc_rtc_flatbuffers
  INCLUDE_PREFIX
  robot_comm/serialization/flatbuffers
  SCHEMAS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/robot_comm/serialization/flatbuffers/mc_rtc_msgs.fbs"
)
target_link_libraries(mc_rtc_flatbuffers INTERFACE flatbuffers::flatbuffers)

# ── Protobuf code generation (optional) ──────────────────────────────────────
if(protobuf_FOUND AND WITH_PROTOBUF)
  add_library(
    proto-mc-rtc OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/include/robot_comm/serialization/protobuf/mc_rtc_msgs.proto"
  )
  target_link_libraries(proto-mc-rtc PUBLIC protobuf::libprotobuf)
  set_target_properties(proto-mc-rtc PROPERTIES POSITION_INDEPENDENT_CODE ON)
  protobuf_generate(
    TARGET proto-mc-rtc IMPORT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/include/robot_comm/serialization/protobuf"
  )
  install(
    TARGETS proto-mc-rtc
    EXPORT "${PROJECT_NAME}Targets"
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
else()
  message(
    WARNING
      "Protobuf message will not be generated (WITH_PROTOBUF=${WITH_PROTOBUF})"
  )
endif()

# ── Build ─────────────────────────────────────────────────────────────────────
add_subdirectory(src)

if(BUILD_TESTING)
  jrl_include_ctest()
  add_subdirectory(tests)
endif()

install(DIRECTORY include/ DESTINATION include)

jrl_print_dependencies_summary()
jrl_print_options_summary()

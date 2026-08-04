cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME mc_fleet)
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
jrl_option(BUILD_TESTING "Build tests" OFF)

# ── Dependencies ──────────────────────────────────────────────────────────────
jrl_find_package(Boost REQUIRED COMPONENTS program_options thread)
jrl_find_package(mc_rtc REQUIRED)
jrl_find_package(Threads REQUIRED)

find_program(SETCAP setcap)
find_program(SUDO sudo)
if(NOT SUDO)
  message(FATAL_ERROR "sudo must be available on the system")
else()
  message(STATUS "sudo command: ${SUDO}")
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

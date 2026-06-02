cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME mc_communication)
set(PROJECT_VERSION 1.0.0)
project(
  ${PROJECT_NAME}
  LANGUAGES CXX
  VERSION ${PROJECT_VERSION}
)

set(CXX_DISABLE_WERROR 1)
set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

option(BUILD_TESTING "Build tests" OFF)
option(WITH_PROTOBUF "Build Protobuf message" OFF)

# Dependencies
find_package(FlatBuffers REQUIRED)
find_package(mc_rtc REQUIRED) # TODO remove used for logs
find_package(zenohc REQUIRED)
find_package(zenohcxx REQUIRED)
find_package(protobuf QUIET)
####

# Flatbuffers
flatbuffers_generate_headers(
  TARGET mc_rtc_flatbuffers
  INCLUDE_PREFIX mc_communication/serialization/flatbuffers

  SCHEMAS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/mc_communication/serialization/flatbuffers/mc_rtc_msgs.fbs"
)

# Protobuf
if(${protobuf_FOUND} AND WITH_PROTOBUF)
  add_library(proto-mc-rtc OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/include/mc_communication/serialization/protobuf/mc_rtc_msgs.proto")

  target_link_libraries(proto-mc-rtc PUBLIC protobuf::libprotobuf)
  set_target_properties(proto-mc-rtc PROPERTIES POSITION_INDEPENDENT_CODE ON)

  target_include_directories(proto-mc-rtc PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/serialization/protobuf>")

  protobuf_generate+(
    TARGET proto-mc-rtc
    IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}/include/mc_communication/serialization/protobuf"
    PROTOC_OUT_DIR "${CMAKE_CURRENT_LIST_DIR}/include/mc_communication/serialization/protobuf")

  install(
    TARGETS proto-mc-rtc
    EXPORT "${PROJECT_NAME}Targets"
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

else()
  message(WARNING "Protobuf message will not be generated, WITH_PROTOBUF : ${WITH_PROTOBUF}")
endif()

# Add Subdirectories
add_subdirectory(src)

# Optional Testing
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(DIRECTORY include/ DESTINATION include)

### CMake macro
macro(add_protocol NAME)
  file(
    GLOB_RECURSE
    PROTOCOL_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mc_communication/${NAME}/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mc_communication/${NAME}/*.h
  )

  add_library(${NAME} SHARED)
  add_library(${PROJECT_NAME}::${NAME} ALIAS ${NAME})

  target_link_libraries(${NAME} PUBLIC mc_communication_interface)

  target_sources(${NAME} PRIVATE ${PROTOCOL_SOURCES})

  target_include_directories(
    ${NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                   $<INSTALL_INTERFACE:include>
  )

  target_link_libraries(mc_communication PUBLIC ${NAME})

  install(
    TARGETS ${NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

endmacro()

### Serialization
macro(add_serializer NAME)
  add_library(${NAME} SHARED ${ARGN})

  target_include_directories(
    ${NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                   $<INSTALL_INTERFACE:include>
  )

  target_link_libraries(mc_serializer INTERFACE ${NAME})

  install(
    TARGETS ${NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endmacro()

add_library(mc_serializer INTERFACE)
add_library(${PROJECT_NAME}::communication::serializer ALIAS mc_serializer)

add_serializer(
  mc-flatbuffers serialization/FlatBufferSerializer.cpp
  ../include/mc_communication/serialization/FlatbufferSerializer.h
)

target_link_libraries(mc-flatbuffers PUBLIC mc_rtc_flatbuffers)

if(TARGET proto-mc-rtc)
  add_serializer(
    mc-protobuf serialization/ProtobufSerializer.cpp
    ../include/mc_communication/serialization/ProtobufSerializer.h
  )
  target_link_libraries(mc-protobuf PUBLIC proto-mc-rtc)

  message(STATUS "mc_communication: Protobuf serializer enabled")
endif()

###  Communication Interface
set(COMMUNICATION_INTERFACE_SRC Communication.cpp)

set(COMMUNICATION_INTERFACE_HDR ../include/mc_communication/Communication.h)

add_library(
  mc_communication_interface SHARED ${COMMUNICATION_INTERFACE_SRC}
                                    ${COMMUNICATION_INTERFACE_HDR}
)
add_library(
  ${PROJECT_NAME}::mc_communication_interface ALIAS mc_communication_interface
)

target_link_libraries(
  mc_communication_interface PUBLIC mc_rtc::mc_control mc_serializer
)

target_include_directories(
  mc_communication_interface
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

### Communication Factory

set(COMMUNICATION_FACTORY_SRC
    CommunicationFactory.cpp ../include/mc_communication/CommunicationFactory.h
)

add_library(mc_communication SHARED ${COMMUNICATION_FACTORY_SRC})
add_library(${PROJECT_NAME}::mc_communication ALIAS mc_communication)

target_link_libraries(mc_communication PUBLIC mc_rtc::mc_control mc_serializer)

target_include_directories(
  mc_communication
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
         $<INSTALL_INTERFACE:include>
)

### Add Protocols

add_protocol(zenoh)
target_link_libraries(zenoh PUBLIC zenohcxx::zenohc)

# add_protocol(shm)

install(
  TARGETS mc_communication mc_communication_interface
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

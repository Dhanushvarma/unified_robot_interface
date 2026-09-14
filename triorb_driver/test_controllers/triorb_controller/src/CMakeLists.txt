set(controller_SRC TriOrbController.cpp)

set(controller_HDR TriOrbController.h)

add_controller(${PROJECT_NAME} ${controller_SRC} ${controller_HDR})
set_target_properties(
  ${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DTriOrbController_EXPORTS"
)

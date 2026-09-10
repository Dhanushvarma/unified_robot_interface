set(controller_SRC CallMController.cpp)

set(controller_HDR CallMController.h)

add_controller(${PROJECT_NAME} ${controller_SRC} ${controller_HDR})
set_target_properties(
  ${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DCallMController_EXPORTS"
)

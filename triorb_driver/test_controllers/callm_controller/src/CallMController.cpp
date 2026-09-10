#include "CallMController.h"

#include <mc_rbdyn/RobotLoader.h>

CallMController::CallMController(mc_rbdyn::RobotModulePtr rm, double dt, const mc_rtc::Configuration & config)
: mc_control::MCController({rm, mc_rbdyn::RobotLoader::get_robot_module("TriOrb")}, dt)
{
  solver().addConstraintSet(contactConstraint);
  solver().addConstraintSet(kinematicsConstraint);
  solver().addTask(postureTask);
  solver().setContacts({{}});

  mc_rtc::log::success("CallMController init done ");
}

bool CallMController::run()
{
  return mc_control::MCController::run();
}

void CallMController::reset(const mc_control::ControllerResetData & reset_data)
{
  mc_control::MCController::reset(reset_data);

  const auto baseXIndex = robot("triorb").jointIndexByName("base_x");
  const auto baseYIndex = robot("triorb").jointIndexByName("base_y");

  gui()->addElement({"TriOrb", "Target"},
                    mc_rtc::gui::NumberInput(
                        "x [m]", [this, baseXIndex]() { return triorbPostureTask_->posture()[baseXIndex][0]; },
                        [this, baseXIndex](double value)
                        {
                          auto posture = triorbPostureTask_->posture();
                          posture[baseXIndex][0] = value;
                          triorbPostureTask_->posture(posture);
                        }),
                    mc_rtc::gui::NumberInput(
                        "y [m]", [this, baseYIndex]() { return triorbPostureTask_->posture()[baseYIndex][0]; },
                        [this, baseYIndex](double value)
                        {
                          auto posture = triorbPostureTask_->posture();
                          posture[baseYIndex][0] = value;
                          triorbPostureTask_->posture(posture);
                        }));
}

CONTROLLER_CONSTRUCTOR("CallMController", CallMController)

#include "TriOrbController.h"

TriOrbController::TriOrbController(mc_rbdyn::RobotModulePtr rm, double dt, const mc_rtc::Configuration & config)
: mc_control::MCController(rm, dt)
{
  solver().addConstraintSet(contactConstraint);
  solver().addConstraintSet(kinematicsConstraint);
  solver().addTask(postureTask);
  solver().setContacts({{}});

  const auto baseXIndex = robot().jointIndexByName("base_x");
  const auto baseYIndex = robot().jointIndexByName("base_y");

  gui()->addElement({"TriOrb", "Target"},
                    mc_rtc::gui::NumberInput(
                        "x [m]", [this, baseXIndex]() { return postureTask->posture()[baseXIndex][0]; },
                        [this, baseXIndex](double value)
                        {
                          auto posture = postureTask->posture();
                          posture[baseXIndex][0] = value;
                          postureTask->posture(posture);
                        }),
                    mc_rtc::gui::NumberInput(
                        "y [m]", [this, baseYIndex]() { return postureTask->posture()[baseYIndex][0]; },
                        [this, baseYIndex](double value)
                        {
                          auto posture = postureTask->posture();
                          posture[baseYIndex][0] = value;
                          postureTask->posture(posture);
                        }));

  mc_rtc::log::success("TriOrbController init done ");
}

bool TriOrbController::run()
{
  return mc_control::MCController::run();
}

void TriOrbController::reset(const mc_control::ControllerResetData & reset_data)
{
  mc_control::MCController::reset(reset_data);
}

CONTROLLER_CONSTRUCTOR("TriOrbController", TriOrbController)

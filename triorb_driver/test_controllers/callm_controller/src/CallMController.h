#pragma once

#include <mc_control/mc_controller.h>
#include <mc_tasks/PostureTask.h>

#include "api.h"

struct CallMController_DLLAPI CallMController : public mc_control::MCController
{
  CallMController(mc_rbdyn::RobotModulePtr rm, double dt, const mc_rtc::Configuration & config);

  bool run() override;

  void reset(const mc_control::ControllerResetData & reset_data) override;

private:
  std::shared_ptr<mc_tasks::PostureTask> triorbPostureTask_;
};

// #pragma once

// #include <mc_robot_interface/RobotDriver.h>
// #include <ur_rtde/rtde_control_interface.h>
// #include <ur_rtde/rtde_receive_interface.h>

// namespace mc_rtde
// {
// struct RobotDriverRTDE : public mc_rtc::RobotDriver
// {
//   RobotDriverRTDE(const std::string & ip, const uint16_t & port = 0) : RobotDriver(ip, port)
//   {
//     mc_rtc::log::info("ur_rtde_receive_ = new ur_rtde::RTDEReceiveInterface(ip, 500, {}, false, false, 90);");
//     mc_rtc::log::info("ur_rtde_control_ = new ur_rtde::RTDEControlInterface(ip, 500, flags_, 50002, 85);");

//     // ur_rtde_receive_ = std::make_unique<ur_rtde::RTDEReceiveInterface>(ip, 500, {}, false, false, 90);
//     // ur_rtde_control_ = std::make_unique<ur_rtde::RTDEControlInterface>(ip, 500, flags_, 50002, 85);
//   };

//   void sync() override {}
//   void setDataRead() override {}

//   std::vector<double> getPosition() override
//   {
//     return ur_rtde_receive_->getActualQ();
//   }
//   std::vector<double> getVelocity() override
//   {
//     return ur_rtde_receive_->getActualQd();
//   }
//   std::vector<double> getTorque() override
//   {
//     return ur_rtde_control_->getJointTorques();
//   }

//   void setPosition(const std::vector<double> & command) override
//   {
//     auto start_t = ur_rtde_control_->initPeriod();
//     ur_rtde_control_->servoJ(command, servoj_velocity_, servoj_acceleration_, dt_, lookahead_time_, servoj_gain_);
//     ur_rtde_control_->waitPeriod(start_t);
//   }
//   void setVelocity(const std::vector<double> & command) override
//   {
//     auto start_t = ur_rtde_control_->initPeriod();
//     ur_rtde_control_->speedJ(command, speedj_acceleration_, dt_);
//     ur_rtde_control_->waitPeriod(start_t);
//   }

//   void setTorque(const std::vector<double> & command) override
//   {
//     mc_rtc::log::error("[ur_rtde] setTorque() is not supported");
//   }

//   bool freeDrive(bool enable) override
//   {
//     if(enable)
//     {
//       return ur_rtde_control_->freedriveMode();
//     }

//     return !ur_rtde_control_->endFreedriveMode();
//   }

// private:
//   uint16_t flags_ = ur_rtde::RTDEControlInterface::FLAG_VERBOSE | ur_rtde::RTDEControlInterface::FLAG_UPLOAD_SCRIPT;

//   /* Communication information with a real robot */
//   std::unique_ptr<ur_rtde::RTDEControlInterface> ur_rtde_control_;
//   std::unique_ptr<ur_rtde::RTDEReceiveInterface> ur_rtde_receive_;

//   // Parameters
//   const double dt_ = 0.002;
//   const double lookahead_time_ = 0.03;
//   const double servoj_acceleration_ = 0.01;
//   const double servoj_velocity_ = 0.05;
//   const double servoj_gain_ = 100;
//   const double speedj_acceleration_ = 0.5;
// };

// } // namespace mc_rtde

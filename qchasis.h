/**
 * This file is created by qiufuyu, TEAM 123A
 * 
 * This class provides an abstract layer for a whole complex chasis controlling process
 * using liberaries:
 *  -- lemlib
 *  -- okapi
 * 
 * Copyright: qiufuyu, TEAM 123A
 * 
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _H_QCHASIS
#define _H_QCHASIS
#include"api.h"
#include"okapi/api.hpp"
#include"pros/apix.h"
#include"pal/gterm.h"
#include"lemlib/api.hpp"
#include<vector>
#include<string>
#include<memory>

#define IS_ALL_AWP

//#define IS_RIGHT_AUTO

#define QCHASIS_TRI // 是否启用6电机模式
using namespace okapi;

using CTRL = okapi::ControllerDigital;
namespace QChasisConfig
{
const int LEFT_WHEEL_FRONT = -4;
const int LEFT_WHEEL_MID = -2;
const int LEFT_WHEEL_BACK = -1;
const int RIGHT_WHEEL_FRONT = 13;
const int RIGHT_WHEEL_MID = 12;
const int RIGHT_WHEEL_BACK = 11;

//const int  GYRO_PORT = 13;
const int GYRO_PORT = 3;
const float TRACK_WIDTH = 11;
const float WHEEL_DIAMETER =2.75;
const int WHEEL_RPM = 450 ;//360;  //36->84
const pros::motor_gearset_e_t GEAREST_BOX = pros::E_MOTOR_GEARSET_06;
const okapi::AbstractMotor::gearset OKAPI_GEAREST = okapi::AbstractMotor::gearset::blue;
const okapi::ChassisScales CHASIS_SCALE = {{2.75_in,11_in}, QChasisConfig::WHEEL_RPM};

const float LINEAR_KP =8;
const float LINEAR_KD =30;

const float ANGLE_KP =8;
const float ANGLE_KD =60;


const float SHOOT_DIST = 225.35f;
};


class qchasis : std::enable_shared_from_this<qchasis>
{
private:
  std::shared_ptr<okapi::ChassisController> drive_chasis;
  std::shared_ptr<lemlib::Chassis> auto_chasis;
  std::unique_ptr<pros::Task> printing_t;
  std::unique_ptr<pros::Task> async_action;
  std::shared_ptr<okapi::Controller> m_controller;
  lemlib::Pose m_goal= {0,0,0};
  std::shared_ptr<okapi::AsyncMotionProfileController> motion_profiles;

#ifdef IS_RIGHT_AUTO
  float base_theta = 0;
#else
  float base_theta = -90;
#endif
  bool fst_move = false;
  bool is_calib = false;
  bool m_is_arc = true;
  bool is_left_auto = true;

  float avail_goal_dist = QChasisConfig::SHOOT_DIST;

  pros::Motor left_front_motor {std::abs(QChasisConfig::LEFT_WHEEL_FRONT), QChasisConfig::GEAREST_BOX, QChasisConfig::LEFT_WHEEL_FRONT<0}; // port 1, blue gearbox, not reversed
  pros::Motor left_mid_motor{std::abs(QChasisConfig::LEFT_WHEEL_MID), QChasisConfig::GEAREST_BOX, QChasisConfig::LEFT_WHEEL_MID<0}; // port 2, green gearbox, not reversed
  pros::Motor left_back_motor{std::abs(QChasisConfig::LEFT_WHEEL_BACK), QChasisConfig::GEAREST_BOX, QChasisConfig::LEFT_WHEEL_BACK<0}; // port 2, green gearbox, not reversed
  pros::Motor right_front_motor{std::abs(QChasisConfig::RIGHT_WHEEL_FRONT), QChasisConfig::GEAREST_BOX, QChasisConfig::RIGHT_WHEEL_FRONT<0  }; // port 1, blue gearbox, not reversed
  pros::Motor right_mid_motor{std::abs(QChasisConfig::RIGHT_WHEEL_MID), QChasisConfig::GEAREST_BOX, QChasisConfig::RIGHT_WHEEL_MID<0}; // port 2, green gearbox, not reversed
  pros::Motor right_back_motor{std::abs(QChasisConfig::RIGHT_WHEEL_BACK), QChasisConfig::GEAREST_BOX, QChasisConfig::RIGHT_WHEEL_BACK<0}; // port 2, green gearbox, not reversed

#ifdef QCHASIS_TRI
  pros::MotorGroup left_side_motors{{left_front_motor,left_mid_motor, left_back_motor}};
  pros::MotorGroup right_side_motors{{right_front_motor,right_mid_motor, right_back_motor}};
#else
  pros::MotorGroup left_side_motors{{left_front_motor, left_back_motor}};
  pros::MotorGroup right_side_motors{{right_front_motor, right_back_motor}};
#endif
  pros::MotorGroup left_side_track{{left_front_motor, left_back_motor}};
  pros::MotorGroup right_side_track{{right_front_motor, right_back_motor}};
  pros::Imu inertial_sensor{QChasisConfig::GYRO_PORT}; // port 2
  lemlib::Drivetrain_t drivetrain {
    &left_side_motors, // left drivetrain motors
    &right_side_motors, // right drivetrain motors
    QChasisConfig::TRACK_WIDTH, // track width = 26cm
    QChasisConfig::WHEEL_DIAMETER, // wheel diameter (inch)
    QChasisConfig::WHEEL_RPM // wheel rpm (36->60)
  };
  lemlib::TrackingWheel track_vert_1{&left_side_track,QChasisConfig::WHEEL_DIAMETER,QChasisConfig::TRACK_WIDTH / 2.0f, QChasisConfig::WHEEL_RPM};
  lemlib::TrackingWheel track_vert_2{&right_side_track,QChasisConfig::WHEEL_DIAMETER,QChasisConfig::TRACK_WIDTH / 2.0f, QChasisConfig::WHEEL_RPM};

  lemlib::OdomSensors_t sensors {
    nullptr,
    nullptr,
    nullptr, // horizontal tracking wheel 1
    nullptr, // we don't have a second tracking wheel, so we set it to nullptr
    &inertial_sensor // inertial sensor
  };
  lemlib::ChassisController_t lateralController {
    QChasisConfig::LINEAR_KP, // kP
    QChasisConfig::LINEAR_KD, // kD
    1, // smallErrorRange
    100, // smallErrorTimeout
    3, // largeErrorRange
    500, // largeErrorTimeout
    5 // slew rate
};
 
// turning PID
lemlib::ChassisController_t angularController {
    QChasisConfig::ANGLE_KP, // kP
    QChasisConfig::ANGLE_KD, // kD
    1, // smallErrorRange
    100, // smallErrorTimeout
    3, // largeErrorRange
    500, // largeErrorTimeout
    40 // slew rate
};
public:

    /**
     * Construct default config for qchasis
     * 
     */
    qchasis(bool is_calib, bool is_arc, bool is_left_auto = true);

    /**
     * It should be called for every 10ms
    */
    void tickUpdate();

    /**
     * Caliberate the Odometry system
     * 
     * NOTICE: If the controller SHAKED FOR A LONG TIME, it means `fail to reset the GYRO`!!!
     * NOTICE: IF the controller SHAKED JUST ONCE, it means `GYRO Rests OK` !!!
    */
    void caliberate();

    bool isMoved(){return fst_move;}

    void setGyroHeading(double angle);

    std::shared_ptr<okapi::ChassisController> getDriver();

    std::shared_ptr<lemlib::Chassis> getAutoDriver();

    std::shared_ptr<okapi::Controller> getController();

    std::shared_ptr<okapi::AsyncMotionProfileController> getMotionProfile(){return motion_profiles;}

    qchasis& addGoal(lemlib::Pose g);

    qchasis& headToGoal(int timeout = 800);

    void trigAsyncAction(std::function<void()> act);

    bool getIsLeftAuto(){return is_left_auto;}

    void calibDirection(float direction);

    bool checkButton(CTRL key);

    qchasis& driveTimedRun(float time_sec, int pct);

    qchasis& driveMoveTo(float dx, float dy, double timeout = 800,bool is_abs = true );

    /**
     * Unit : cm
    */
    qchasis& driveForward(float distance, int timeout= 800);

    /**
     * Unit : degree
    */
    qchasis& driveTurn(float angle, int timeout = 800);

    qchasis& driveDeltaTurn(float delta, int timeout = 800);

    qchasis& resetPose(float x,float y);

    qchasis& driveCurve(const char* name,float delta, int timeout);

    qchasis& driveTurnTo(float x, float y, int timeout = 800);

    qchasis& driveDelay(int timeout);

    float getGoalDistance();

    qchasis& driveGoalDistance(float distance = -1, int timeout=800, float err_range = 5);

    ~qchasis(){};
};


#endif
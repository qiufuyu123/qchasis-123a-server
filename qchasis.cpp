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

#include"qchasis.h"
#include"qtimer.h"
using namespace okapi;

qchasis::qchasis(bool is_calib, bool is_arc, bool is_left)
{
    is_left_auto = is_left;
    drive_chasis = ChassisControllerBuilder().withMotors(

// #ifdef QCHASIS_TRI
//         {-QChasisConfig::LEFT_WHEEL_FRONT,-QChasisConfig::LEFT_WHEEL_MID,-QChasisConfig::LEFT_WHEEL_BACK},
//         {QChasisConfig::RIGHT_WHEEL_FRONT,QChasisConfig::RIGHT_WHEEL_MID,QChasisConfig::RIGHT_WHEEL_BACK}
// #else
//         {-QChasisConfig::LEFT_WHEEL_FRONT,-QChasisConfig::LEFT_WHEEL_BACK},
//         {QChasisConfig::RIGHT_WHEEL_FRONT,QChasisConfig::RIGHT_WHEEL_BACK}
// #endif
#ifdef QCHASIS_TRI
        {QChasisConfig::LEFT_WHEEL_FRONT,QChasisConfig::LEFT_WHEEL_MID,QChasisConfig::LEFT_WHEEL_BACK},
        {QChasisConfig::RIGHT_WHEEL_FRONT,QChasisConfig::RIGHT_WHEEL_MID,QChasisConfig::RIGHT_WHEEL_BACK}
#else
        {QChasisConfig::LEFT_WHEEL_FRONT,QChasisConfig::LEFT_WHEEL_BACK},
        {QChasisConfig::RIGHT_WHEEL_FRONT,QChasisConfig::RIGHT_WHEEL_BACK}
#endif
    )
    .withDimensions(QChasisConfig::OKAPI_GEAREST, QChasisConfig::CHASIS_SCALE)
    .build();

    // auto_chasis = std::make_shared<lemlib::Chassis>(drivetrain,lateralController,angularController,sensors);
    // if(is_calib)
    //     caliberate();  // Do caliberate and print time costs
    m_controller = std::make_shared<okapi::Controller>();
    m_is_arc = is_arc;
    printing_t = std::make_unique<pros::Task>(([=]{
        while (1)
        {
            pros::lcd::set_text(7,"Powered By 123A QChasis!");
            pros::delay(1000);
            pros::lcd::set_text(7,"123A is the BEST!");
            pros::delay(1000);
            pros::lcd::set_text(7,"123A Will Win!");
            pros::delay(1000);
        }
        
    }));
//     motion_profiles =
//   AsyncMotionProfileControllerBuilder()
//     .withLimits({
//       1.0, // Maximum linear velocity of the Chassis in m/s
//       2.0, // Maximum linear acceleration of the Chassis in m/s/s
//       10.0 // Maximum linear jerk of the Chassis in m/s/s/s
//     })
//     .withOutput(drive_chasis)
//     .buildMotionProfileController();
    
}
#define M pros::controller_id_e_t::E_CONTROLLER_MASTER
void qchasis::caliberate()
{
    pros::delay(5000);
    qtimer t;
    pros::lcd::set_text(0,"Auto Caliberating ... ");
    /**
     * To avoid failing to reset GYRO in Lemlib
     * We reset GYRO by hand before
    */
    m_controller->setText(2,0,"Please Wait");
    if(inertial_sensor.reset(true)!=1)
    {
        pros::c::controller_clear(M);
        pros::c::controller_set_text(M,0,0,"GYRO ERROR");
        if(errno == ENODEV)
        {
            pros::c::controller_set_text(M,1,0,"BAD PORT");
        }
        else if(errno == ENXIO)
        {
            pros::c::controller_set_text(M,1,0,"NOT A GYRO");
        }
        while (1)
        {
            pros::c::controller_rumble(M,".");
            pros::delay(200);
        }
        
    }
    auto_chasis->calibrate();
    //inertial_sensor.set_euler(pros::c::euler_s_t::)
    uint32_t sec = t.getTime() / 1000;
    //m_controller->rumble(".");
    pros::lcd::set_text(0,std::string("Caliberated in ")+std::to_string(sec)+std::string("(s)"));
    m_controller->setText(2,0,"CALIBERATED!");
    is_calib = true;
    inertial_sensor.set_rotation(base_theta);
}

void qchasis::tickUpdate()
{
    // if(!is_calib)
    //     return;
    auto pose = auto_chasis->getPose();
    pros::lcd::print(3,"x:%lf",pose.x);
    pros::lcd::print(4,"y:%lf",pose.y);
    pros::lcd::print(5,"a:%lf",pose.theta);
    if(1)
    {
        if(m_controller->getAnalog(okapi::ControllerAnalog::leftX) 
        || m_controller->getAnalog(okapi::ControllerAnalog::leftY))
        {
            fst_move = true;
        }
        drive_chasis->getModel()->arcade(-m_controller->getAnalog(okapi::ControllerAnalog::leftX),-m_controller->getAnalog(okapi::ControllerAnalog::leftY));
    }else
    {
        // TODO: 
    }
}
void qchasis::setGyroHeading(double angle)
{
    inertial_sensor.set_rotation(angle);
}
std::shared_ptr<okapi::ChassisController> qchasis::getDriver()
{
    return drive_chasis;
}
std::shared_ptr<lemlib::Chassis> qchasis::getAutoDriver()
{
    return auto_chasis;
}
std::shared_ptr<okapi::Controller> qchasis::getController()
{
    return m_controller;
}

qchasis& qchasis::addGoal(lemlib::Pose g)
{
    m_goal = g;
    return (*this);
}

qchasis& qchasis::headToGoal(int timeout)
{

    auto_chasis->turnTo(m_goal.x,m_goal.y,timeout);
    return (*this);
}

void qchasis::trigAsyncAction(std::function<void()> act)
{
    async_action = nullptr;
    async_action = std::make_unique<pros::Task>(act);
}

void qchasis::calibDirection(float direction)
{
    base_theta = direction;
    inertial_sensor.set_rotation(base_theta);
}

bool qchasis::checkButton(CTRL key)
{
    return m_controller->getDigital(key);
}

qchasis &qchasis::driveTimedRun(float time_sec, int pct)
{
    auto_chasis->timedRun(time_sec,pct);
    auto_chasis->brake();
    return (*this);
}

qchasis &qchasis::driveMoveTo(float dx, float dy, double timeout, bool is_abs)
{
    if(is_abs)
    {
        auto_chasis->moveTo(dx,dy,(int)timeout);
    }else
    {
        lemlib::Pose o = auto_chasis->getPose();
        auto_chasis->moveTo(dx+o.x,dy+o.y,(int)timeout);
    }
    auto_chasis->brake();
    return (*this);
}
#define IN2CM(x) (x/0.3937007874)
#define CM2IN(x) (x*0.3937007874)
#define DEG2RAD(x) (3.1415926f / 180.0f * x)
qchasis &qchasis::driveForward(float distance, int timeout)
{
    distance = CM2IN(distance);
    float theta = auto_chasis->getPose().theta;
    theta = DEG2RAD(theta);
    lemlib::Pose o = getAutoDriver()->getPose();
    float dx = std::sin(theta) * distance;
    float dy = std::cos(theta) * distance;
    getAutoDriver()->moveTo(o.x + dx, o.y + dy,timeout);
    auto_chasis->brake();

    return (*this);
}

qchasis &qchasis::driveTurn(float angle, int timeout)
{
    auto_chasis->turnAngle(angle,timeout);
    auto_chasis->brake();

    return (*this);
}

qchasis &qchasis::driveDeltaTurn(float delta, int timeout)
{
    auto_chasis->turnAngle(auto_chasis->getPose().theta + delta,timeout);
    auto_chasis->brake();

    return (*this);
}

qchasis &qchasis::resetPose(float x, float y)
{
    auto_chasis->setPose(x,y,auto_chasis->getPose().theta);
    return (*this);
}

qchasis &qchasis::driveCurve(const char* name, float delta, int timeout)
{
    auto_chasis->follow(name,timeout,delta,false,200);
    return (*this);
}

qchasis &qchasis::driveTurnTo(float x, float y, int timeout)
{
    auto_chasis->turnTo(x,y,timeout);
    auto_chasis->brake();
    return (*this);
}

qchasis &qchasis::driveDelay(int timeout)
{
    pros::delay(timeout);
    return (*this);
}

float qchasis::getGoalDistance()
{   
    lemlib::Pose p = auto_chasis->getPose();
    return IN2CM(std::sqrt(std::pow(m_goal.x - p.x,2) + std::pow(m_goal.y - p.y,2)));
}

qchasis &qchasis::driveGoalDistance(float distance, int timeout, float err_range)
{
    if(distance == -1)
        distance = avail_goal_dist;
    float now = getGoalDistance();
    float err = now -distance;
    if(std::abs(err) <= err_range)
        return (*this);
    driveForward(err,timeout);
    driveDelay(20);
    return (*this);
}
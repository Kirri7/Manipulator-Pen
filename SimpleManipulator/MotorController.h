#pragma once

#include <Arduino.h>
#include <GyverPlanner.h> // Make sure this library is installed

// Declare extern objects defined in .cpp
extern Stepper<STEPPER2WIRE> MTR1;
extern Stepper<STEPPER2WIRE> MTR2;
extern GPlanner<STEPPER2WIRE, 1> planner_mtr1;
extern GPlanner<STEPPER2WIRE, 1> planner_mtr2;

extern void initMotors();
extern void updateMotors();
extern int angleToIndex(float angle);
extern void processMotorLogic();
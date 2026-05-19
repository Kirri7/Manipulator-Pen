#include "MotorController.h"
#include "config.h"

// Global Steppers and Planners
Stepper<STEPPER2WIRE> MTR1(19, 21);
Stepper<STEPPER2WIRE> MTR2(5, 18);
GPlanner<STEPPER2WIRE, 1> planner_mtr1;
GPlanner<STEPPER2WIRE, 1> planner_mtr2;

// State variables
bool state_home_0 = 1;
bool state_home_1 = 1;
bool state_home_encoder_0 = 0;
bool state_home_encoder_1 = 0;
bool last_target = 0;
bool state_target = 0;

bool pState_0 = false;
bool pStateSwitch_0 = false;
bool pState_1 = false;
bool hold_0 = 0;
bool hold_1 = 0;

bool Z1State = 0;
//int count = (pointAm) / 2 + 2;
//int count2 = (pointAm) / 2 + 2;
int count = 31;
int count2 = 31;
int gstep = 1;  // шаг перемещения по массиву точек

void initMotors() {
    pinMode(Z_Pin, INPUT_PULLUP);
    pinMode(button1Pin, INPUT);
    pinMode(button2Pin, INPUT);
    pinMode(button3Pin, INPUT);
    pinMode(button4Pin, INPUT);
    pinMode(LIMSW_X, INPUT_PULLUP);

    // добавляем шаговики на оси
    planner_mtr2.addStepper(0, MTR2);
    planner_mtr1.addStepper(0, MTR1);

    planner_mtr2.setAcceleration(600);
    planner_mtr2.setMaxSpeed(1500);
    planner_mtr1.setAcceleration(600);
    planner_mtr1.setMaxSpeed(1500);

    planner_mtr2.setSpeed(0, 500);
    planner_mtr1.setSpeed(0, 500);

    planner_mtr2.stop(); planner_mtr2.reset(); planner_mtr2.resume();
    planner_mtr1.stop(); planner_mtr1.reset(); planner_mtr1.resume();
}

void updateMotors() {
    // здесь происходит движение моторов, вызывать как можно чаще
    planner_mtr2.tick();
    planner_mtr1.tick();
}

void processMotorLogic() {
    static uint32_t tmr4 = 0;
    static uint32_t tmr5 = 0;
    
    bool state_switch_0 = !digitalRead(LIMSW_X);
    bool state_switch_1 = !digitalRead(Z_Pin);

    /*
        //Обработка концевика 0
        static uint32_t tmr4;
        if (pState_0 != state_switch_0 && millis() - tmr4 >= BTN_DEB) {
            tmr4 = millis();
            pState_0 = state_switch_0;
            hold_0 = false;  // сброс флага удержания
            if (state_switch_0) {
                planner_mtr2.setSpeed(0,0);
                planner_mtr2.stop();
                planner_mtr2.reset();
                planner_mtr2.resume();
                if (planner_mtr2.ready()) {
                    planner_mtr2.setTarget(path2[0]);
                    state_home_0 = 1;
                    state_home_encoder_0 = 1;
                }
            }
        }
        
        
        //Обработка концевика 1
        static uint32_t tmr5;
        if (pState_1 != state_switch_1 && state_home_encoder_1 == 0 && millis() - tmr5 >= BTN_DEB) {
        tmr5 = millis();
        pState_1 = state_switch_1;
        hold_1 = false;  // сброс флага удержания
        if (state_switch_1) {
            planner_mtr1.setSpeed(0,0);
            planner_mtr1.stop();
            planner_mtr1.reset();
            planner_mtr1.resume();
            if (planner_mtr1.ready()) {
        planner_mtr1.setTarget(path1[0]);
        state_home_1 = 1;
        state_home_encoder_1 = 1;
        }
        }
        }
        */

    bool btn3 = digitalRead(button3Pin); // Backward/Down
    bool btn4 = digitalRead(button4Pin); // Forward/Up
    bool btn1 = digitalRead(button1Pin); // Right
    bool btn2 = digitalRead(button2Pin); // Left

    // Axis 0 (Up/Down - MTR2)
    bool axis0Active = btn3 || btn4 || g_Input.down || g_Input.up || state_home_0;
    
    if (axis0Active && planner_mtr2.ready()) {
        // Serial.print(g_Input.left); 
        // Serial.print(g_Input.right);
        // Serial.print(g_Input.up);
        // Serial.println(g_Input.down);
        
        // INITIAL HOMOING (Runs once at startup)
        if (state_home_0) { 
            state_home_0 = false;
            
            // Move to a known middle position
            int startPosition = (pointAm / 2) + 2; 
            planner_mtr2.setTarget(path2[startPosition]);
            count = startPosition;
            
            Serial.print("Axis 0: HOMED to middle: ");
            Serial.println((pointAm/2)+2);
        } 
        // NORMAL OPERATION (Manual Control)
        else {
            planner_mtr2.setTarget(path2[count]);
            
            if (btn4 || g_Input.up) {
                if (count < pointAm - 1) {
                    count += gstep;
                }
                // else count = pointAm-1;
                //planner_mtr2.setTarget(path[count+1]);  // загружаем новую точку (начнётся с 0)
            }

            if (btn3 || g_Input.down) {
                if (count > 3) { // 0 слишком близко к опоре
                    count -= gstep;
                }
                // else count = 3;
                //planner_mtr2.setTarget(path[count]);  // загружаем новую точку (начнётся с 0)
            }

            Serial.print("up/down, count:"); Serial.println(count);
        }
    }

    // Axis 1 (Left/Right - MTR1)
    bool axis1Active = btn1 || btn2 || g_Input.left || g_Input.right || state_home_1;
    
    if (axis1Active && planner_mtr1.ready()) {
        // INITIAL HOMOING (Runs once at startup)
        if (state_home_1) {
            state_home_1 = false;
            
            // Move to a known middle position
            int startPosition = (pointAm / 2) + 2; 
            planner_mtr1.setTarget(path1[startPosition]);
            count2 = startPosition;
            
            Serial.print("Axis 1: HOMED to middle: ");
            Serial.println((pointAm/2)+2);
        } 
        // NORMAL OPERATION (Manual Control)
        else {
            planner_mtr1.setTarget(path1[count2]);
            
            if (btn2 || g_Input.left) {
                if (count2 < pointAm - 1) count2 += gstep;
                // else count2 = pointAm;
            }

            //?возвращает обратно
            if (btn1 || g_Input.right) {
                if (count2 > 4) count2 -= gstep;
                // else count2 = 4;  //0
            }
            
            Serial.print("left/right, count2: "); Serial.println(count2);
        }
    }
}
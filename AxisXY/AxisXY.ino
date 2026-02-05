#include "GyverPlanner.h"

// === ПИНЫ === //
constexpr uint8_t button1Pin = A2;
constexpr uint8_t button2Pin = A3;
constexpr uint8_t button3Pin = A4;
constexpr uint8_t button4Pin = A5;
constexpr uint8_t Z_Pin = 10;

// === КРУГОВОЕ ДВИЖЕНИЕ === //
constexpr uint8_t pointAm = 150;  // количество точек в круге
int R = 100;              //100           // радиус круга
float alfa = 36 * 3.14159 / 180;
float L = 2 * R * sin(alfa / 2);
float L2 = R * sin(alfa / 2);
int dist = 100;
const int path[][2] = {{100, 0}, {0, 0}, {23, -26}, {46, -53}, {69, -79}, {92, -104}, {115, -129}, {138, -154}, {161, -178}, {184, -202}, {207, -226}, {230, -249}, {253, -272}, {276, -295}, {299, -317}, {322, -338}, {345, -360}, {368, -381}, {391, -401}, {414, -421}, {437, -441}, {460, -461}, {483, -480}, {506, -498}, {529, -517}, {552, -535}, {576, -552}, {599, -569}, {622, -586}, {645, -603}, {668, -619}, {691, -635}, {714, -650}, {737, -665}, {760, -680}, {783, -694}, {806, -708}, {829, -722}, {852, -735}, {875, -748}, {898, -760}, {921, -773}, {944, -784}, {967, -796}, {990, -807}, {1013, -818}, {1036, -828}, {1059, -838}, {1082, -848}, {1105, -857}, {1129, -866}, {1152, -875}, {1175, -883}, {1198, -891}, {1221, -899}, {1244, -906}, {1267, -913}, {1290, -919}, {1313, -926}, {1336, -931}, {1359, -937}, {1382, -942}, {1405, -947}, {1428, -951}, {1451, -956}, {1474, -959}, {1497, -963}, {1520, -966}, {1543, -969}, {1566, -971}, {1589, -973}, {1612, -975}, {1635, -976}, {1658, -977}, {1682, -978}, {1705, -978}, {1728, -978}, {1751, -978}, {1774, -977}, {1797, -976}, {1820, -975}, {1843, -973}, {1866, -971}, {1889, -969}, {1912, -966}, {1935, -963}, {1958, -959}, {1981, -956}, {2004, -951}, {2027, -947}, {2050, -942}, {2073, -937}, {2096, -931}, {2119, -926}, {2142, -919}, {2165, -913}, {2188, -906}, {2211, -899}, {2235, -891}, {2258, -883}, {2281, -875}, {2304, -866}, {2327, -857}, {2350, -848}, {2373, -838}, {2396, -828}, {2419, -818}, {2442, -807}, {2465, -796}, {2488, -784}, {2511, -773}, {2534, -760}, {2557, -748}, {2580, -735}, {2603, -722}, {2626, -708}, {2649, -694}, {2672, -680}, {2695, -665}, {2718, -650}, {2741, -635}, {2764, -619}, {2788, -603}, {2811, -586}, {2834, -569}, {2857, -552}, {2880, -535}, {2903, -517}, {2926, -498}, {2949, -480}, {2972, -461}, {2995, -441}, {3018, -421}, {3041, -401}, {3064, -381}, {3087, -360}, {3110, -338}, {3133, -317}, {3156, -295}, {3179, -272}, {3202, -249}, {3225, -226}, {3248, -202}, {3271, -178}, {3294, -154}, {3317, -129}, {3341, -104}, {3364, -79}, {3387, -53}, {3410, -26}, {3433, 0}};
const int path2[][2] ={{77, 0}, {0, 0}, {1716, 880}, {1693, 859}, {1670, 840}, {1647, 820}, {1625, 801}, {1602, 782}, {1579, 764}, {1556, 745}, {1533, 727}, {1510, 710}, {1487, 692}, {1464, 675}, {1441, 658}, {1419, 642}, {1396, 625}, {1373, 609}, {1350, 594}, {1327, 578}, {1304, 563}, {1281, 548}, {1258, 534}, {1235, 519}, {1213, 505}, {1190, 492}, {1167, 478}, {1144, 465}, {1121, 452}, {1098, 439}, {1075, 427}, {1052, 415}, {1029, 403}, {1007, 392}, {984, 380}, {961, 369}, {938, 359}, {915, 348}, {892, 338}, {869, 328}, {846, 318}, {823, 309}, {801, 300}, {778, 291}, {755, 282}, {732, 274}, {709, 266}, {686, 258}, {663, 251}, {640, 243}, {617, 236}, {595, 229}, {572, 223}, {549, 217}, {526, 211}, {503, 205}, {480, 200}, {457, 194}, {434, 190}, {411, 185}, {389, 181}, {366, 176}, {343, 173}, {320, 169}, {297, 166}, {274, 162}, {251, 160}, {228, 157}, {205, 155}, {183, 153}, {160, 151}, {137, 149}, {114, 148}, {91, 147}, {68, 146}, {45, 146}, {22, 145}, {0, 145}, {-22, 146}, {-45, 146}, {-68, 147}, {-91, 148}, {-114, 149}, {-137, 151}, {-160, 153}, {-183, 155}, {-205, 157}, {-228, 160}, {-251, 162}, {-274, 166}, {-297, 169}, {-320, 173}, {-343, 176}, {-366, 181}, {-389, 185}, {-411, 190}, {-434, 194}, {-457, 200}, {-480, 205}, {-503, 211}, {-526, 217}, {-549, 223}, {-572, 229}, {-595, 236}, {-617, 243}, {-640, 251}, {-663, 258}, {-686, 266}, {-709, 274}, {-732, 282}, {-755, 291}, {-778, 300}, {-801, 309}, {-823, 318}, {-846, 328}, {-869, 338}, {-892, 348}, {-915, 359}, {-938, 369}, {-961, 380}, {-984, 392}, {-1007, 403}, {-1029, 415}, {-1052, 427}, {-1075, 439}, {-1098, 452}, {-1121, 465}, {-1144, 478}, {-1167, 492}, {-1190, 505}, {-1213, 519}, {-1235, 534}, {-1258, 548}, {-1281, 563}, {-1304, 578}, {-1327, 594}, {-1350, 609}, {-1373, 625}, {-1396, 642}, {-1419, 658}, {-1441, 675}, {-1464, 692}, {-1487, 710}, {-1510, 727}, {-1533, 745}, {-1556, 764}, {-1579, 782}, {-1602, 801}, {-1625, 820}, {-1647, 840}, {-1670, 859}};

constexpr uint8_t LIM_SW_X = 11;
constexpr uint8_t BTN_DEBOUNCE = 50;
constexpr uint8_t BTN_HOLD = 500; // ?

// === УПРАВЛЕНИЕ ДВИГАТЕЛЯМИ === //
Stepper<STEPPER2WIRE> stepper1(5, 4);
Stepper<STEPPER2WIRE> stepper2(12, 13);
Stepper<STEPPER2WIRE> stepper2_2(12, 13);
Stepper<STEPPER2WIRE> stepper3(3, 2);
GPlanner<STEPPER2WIRE, 2> planner;
GPlanner<STEPPER2WIRE, 2> planner2;

// === СОСТОЯНИЯ КНОПОК И КОНЦЕВИКОВ === //
bool pState_0 = false;
bool pState_1 = false;
bool state_switch_0 = 0;
bool state_switch_1 = 0;
bool hold_0 = 0; // ?
bool hold_1 = 0; // ?

// === СОСТОЯНИЯ СИСТЕМЫ === //
// bool State = false;
int count = (pointAm) / 2 + 2;
int count2 = (pointAm) / 2 + 2;

// === ФЛАГИ ХОМИНГА (ПОИСКА НУЛЯ) === //
bool state_home_0 = 0;
bool state_home_1 = 0;
bool state_home_encoder_0 = 0; // ?
bool state_home_encoder_1 = 0;

// === СОСТОЯНИЯ КНОПОК РУЧНОГО УПРАВЛЕНИЯ === //
bool Status1 = 0;
bool Status2 = 0;
bool Status3 = 0;
bool Status4 = 0;

// === СЕРВИСНЫЕ ПЕРЕМЕННЫЕ === //
char read = 'f'; 


void setup() {
    pinMode(Z_Pin, INPUT_PULLUP);
    pinMode(button1Pin, INPUT);
    pinMode(button2Pin, INPUT);
    pinMode(button3Pin, INPUT);
    pinMode(button4Pin, INPUT);
    pinMode(LIM_SW_X, INPUT_PULLUP);
    
    pinMode(A0, INPUT);
    
    Serial.begin(115200);
    //Serial.println(L);

    // добавляем шаговики на оси
    planner.addStepper(0, stepper1);  // ось 0
    planner.addStepper(1, stepper2_2);  // ось 1
    
    planner2.addStepper(0, stepper3);  // ось 0
    planner2.addStepper(1, stepper2);  // ось 1

    // устанавливаем ускорение и скорость
    planner.setAcceleration(0);
    planner.setMaxSpeed(600);
    
    planner2.setAcceleration(0);
    planner2.setMaxSpeed(600);  //400
    
    // L = L/1.44;
    
    //Serial.println(path[0][0]);
    //Serial.println(path[1][0]);
    
    planner.setSpeed(0, -500);
    planner2.setSpeed(0, 500);  //300
}

void loop() {
    //delay(10);
    // здесь происходит движение моторов, вызывать как можно чаще
    planner.tick();
    planner2.tick();
    
    
    /*  
    if (Serial.available() > 0 && State == false) {
        for (int i = 0; i <= pointAm; i++) {
            Serial.print(path2[i + 1][0]);
            Serial.print(" ");
            Serial.println(path2[i + 1][1]);
        }
        State = true;
    }
    */ 
    
    if (Serial.available() != 0) {
        read = (char)Serial.read();
    }
    
    state_switch_0 = !digitalRead(LIM_SW_X);
    state_switch_1 = !digitalRead(Z_Pin);
    
    
    Status1 = digitalRead(button1Pin);
    Status2 = digitalRead(button2Pin);
    
    Status3 = digitalRead(button3Pin);
    Status4 = digitalRead(button4Pin);
    
    int value = analogRead(A0);
    int moveX_state = decodeState(value); // -1 0 1
    
    Serial.print("Value: ");
    Serial.print(value);
    Serial.print(" → State: ");
    Serial.println(moveX_state);
    
    //   if (moveX_state == 1) {Status1 == LOW;}
    //   else if (moveX_state == -1) {Status2 == LOW;}
    
    //Обработка концевика 0
    static uint32_t tmr4;
    if (pState_0 != state_switch_0 && millis() - tmr4 >= BTN_DEBOUNCE) {
        tmr4 = millis();
        pState_0 = state_switch_0;
        hold_0 = false;  // сброс флага удержания
        if (state_switch_0) {
            planner.setSpeed(0, 0);
            planner.brake();
            planner.reset();
            planner.resume();
            if (planner.ready()) {
                planner.setTarget(path[0]);
                state_home_0 = 1;
                state_home_encoder_0 = 1;
            }
        }
    }
    
    
    //Обработка концевика 1
    static uint32_t tmr5;
    if (pState_1 != state_switch_1 && state_home_encoder_1 == 0 && millis() - tmr5 >= BTN_DEBOUNCE) {
        tmr5 = millis();
        pState_1 = state_switch_1;
        hold_1 = false;  // сброс флага удержания
        if (state_switch_1) {
            planner2.setSpeed(0, 0);
            planner2.brake();
            planner2.reset();
            planner2.resume();
            if (planner2.ready()) {
                planner2.setTarget(path2[1]);
                state_home_1 = 1;
                state_home_encoder_1 = 1;
            }
        }
    }
    
    if (Status3 or Status4 or state_home_0 or read == 'a' or read == 'b') {
        Status1 = 0;
        Status2 = 0;
        if (planner.ready()) {
            if (state_home_0 == 1) {
                state_home_0 = 0;
                planner.reset();
                planner.setTarget(path[(pointAm/2)+2]);
                
            } else if (state_home_0 == 0) {
                
                planner.setTarget(path[count]);
                
                if (Status4 == true or read == 'a') {
                    
                    //planner.setTarget(path[count+1]);  // загружаем новую точку (начнётся с 0)
                    if (count < pointAm-1) {
                        ++count;
                    } else count = pointAm-1;
                }
                
                if (Status3 == true or read == 'b') {
                    if (count > 3) --count;
                    else count = 3;  //0
                    //planner.setTarget(path[count]);  // загружаем новую точку (начнётся с 0)
                }
            }
        }
    }
    
    
    
    
    if (Status1 or Status2 or read == 'c' or read == 'd') {
        Status3 = 0;
        Status4 = 0;
        
        if (planner2.ready()) {
            if (state_home_1 == 1) {
                state_home_1 = 0;
                planner2.reset();
                planner2.setTarget(path2[count2]);
            } else if (state_home_1 == 0) {
                planner2.setTarget(path2[count2]);
                
                
                if (Status2 == true or read=='c') {
                    if (count2 < pointAm) {
                        ++count2;
                    } else count2 = pointAm;
                }
                
                //возвращает обратно
                if (Status1 == true or read=='d') {
                    if (count2 > 4) --count2;
                    else count2 = 4;  //0
                }
            }
        }
    }
}

int decodeState(int analogValue) {
    if (analogValue < 100) return -1;      // ~0V
    else if (analogValue < 600) return 0;  // ~2.5V
    else return 1;                         // ~5V
}


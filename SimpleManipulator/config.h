#pragma once

#include <Arduino.h>
#include <iterator>

// --- MODE SELECTION ---
// Uncomment the line below to select the mode you want to use.
// Only ONE should be defined.
// #define USE_WIFI_MODE
#define USE_BLE_MODE
// If neither is defined, Serial Mode is used by default.

// --- PINS ---
const int button1Pin = 13;
const int button2Pin = 12;
const int button3Pin = 14;
const int button4Pin = 27;
const int Z_Pin = 17;
const int LED_BUILTIN = 2;
const int LIMSW_X = 16;

// --- SETTINGS ---
const int BTN_DEB = 50;
const int BTN_HOLD = 500;

// --- WIFI CREDENTIALS ---
#ifdef USE_WIFI_MODE
inline const char* SSID = "xxx";
inline const char* PASSWORD = "xxx";
#define WIFI_PORT 10000
#endif

// --- BLE SETTINGS ---
#ifdef USE_BLE_MODE
#define REMOTE_SERVICE_UUID "acc0a4a9-f284-4eac-8fa5-d825c55ce64c"
#define REMOTE_CHARACTERISTIC_UUID "fc18c54c-2f23-4c05-84bd-338ca880b786"
#define COMPUTER_SERVICE_UUID "6938e8b6-77d8-44e4-ab9d-d27918908cb8"
#define COMPUTER_CHARACTERISTIC_UUID "e869108c-f2db-4772-a6ba-380a0761ef24"
#endif

// --- PATH DATA ---
// Массивы для шаговых двигателей (по опыту - 50 шагов достаточно мало)
const int32_t path1[][1] = {
{-3000},{-2900},{-2800},{-2700},{-2600},{-2500},{-2400},{-2300},{-2200},{-2100},{-2000},{-1900},{-1800},{-1700},{-1600},{-1500},{-1400},{-1300},{-1200},{-1100},{-1000},{-900},{-800},{-700},{-600},{-500},{-400},{-300},{-200},{-100},{0},{100},{200},{300},{400},{500},{600},{700},{800},{900},{1000},{1100},{1200},{1300},{1400},{1500},{1600},{1700},{1800},{1900},{2000},{2100},{2200},{2300},{2400},{2500},{2600}};

const int32_t path2[][1] = {
{-3000},{-2900},{-2800},{-2700},{-2600},{-2500},{-2400},{-2300},{-2200},{-2100},{-2000},{-1900},{-1800},{-1700},{-1600},{-1500},{-1400},{-1300},{-1200},{-1100},{-1000},{-900},{-800},{-700},{-600},{-500},{-400},{-300},{-200},{-100},{0},{100},{200},{300},{400},{500},{600},{700},{800},{900},{1000},{1100},{1200},{1300},{1400},{1500},{1600},{1700},{1800},{1900},{2000},{2100},{2200},{2300},{2400},{2500},{2600}};

const int pointAm = std::size(path1);
// assert(...)

// предельные значения наклона пульта, дальше которых установка не пойдёт
const float DEG_MAX =  100.0f;

// --- GLOBAL STATE STRUCT ---
struct DirectionState {
  bool left = false;
  bool right = false;
  bool up = false;
  bool down = false;
};

struct TargetAngles {
  float roll = 0.0f;
  float pitch = 0.0f;
  bool newUpdate = false;
};

// Global instance to be updated by communication modules
extern DirectionState g_Input;
extern TargetAngles g_TargetAngles;

// --- EXTERNAL FUNCTION DECLARATIONS ---
extern void initMotors();
extern void updateMotors();
extern void processMotorLogic();

extern void initCommunication();
extern void updateCommunication();

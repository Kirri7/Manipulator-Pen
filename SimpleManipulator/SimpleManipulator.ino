#include "config.h"
#include "MotorController.h"

// Include the specific manager based on config
#ifdef USE_WIFI_MODE
#include "WiFiManager.h"
#define COMMS_INIT wifiManager.init()
#define COMMS_UPDATE wifiManager.update()
#elif defined(USE_BLE_MODE)
#include "BLEManager.h"
#define COMMS_INIT bleManager.init()
#define COMMS_UPDATE bleManager.update()
#else
#include "SerialManager.h"
#define COMMS_INIT serialManager.init()
#define COMMS_UPDATE serialManager.update()
#endif

// Define the global input state here
DirectionState g_Input = {};
TargetAngles g_TargetAngles = {};

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(100);
  
  initMotors();
  COMMS_INIT;
}

void loop() {
  updateMotors();
  COMMS_UPDATE;
  processMotorLogic();
  
  // Small delay to prevent watchdog reset if logic is too slow
  yield(); 
}
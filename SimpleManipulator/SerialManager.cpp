#include "SerialManager.h"

SerialManager serialManager;

void SerialManager::init() {
  // Serial.begin(115200);
}

void SerialManager::update() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    serial_buf[buf_idx++] = cmd;
    // TODO DRY
    if (buf_idx == 4) {
      buf_idx = 0;
      uint32_t value = ((uint32_t)serial_buf[3] << 24) | 
                       ((uint32_t)serial_buf[2] << 16) | 
                       ((uint32_t)serial_buf[1] << 8) | 
                       ((uint32_t)serial_buf[0]);
      g_Input.right = (value & (1 << 0)) != 0;
      g_Input.left  = (value & (1 << 8)) != 0;
      g_Input.up    = (value & (1 << 16)) != 0;
      g_Input.down  = (value & (1 << 24)) != 0;
    }
  }
}
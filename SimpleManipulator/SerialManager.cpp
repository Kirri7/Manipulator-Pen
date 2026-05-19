#include "SerialManager.h"
#include "PacketParser.h"

SerialManager serialManager;

void SerialManager::init() {
  // Serial.begin(115200);
}

void SerialManager::update() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    serial_buf[buf_idx++] = cmd;
    if (buf_idx == 4) {
      buf_idx = 0;
      PacketParser::parseCommand(serial_buf, 4);
    }
  }
}
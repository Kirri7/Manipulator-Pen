#pragma once
#include "config.h"

class SerialManager {
public:
  void init();
  void update();
private:
  uint8_t serial_buf[4];
  uint8_t buf_idx = 0;
};

extern SerialManager serialManager;
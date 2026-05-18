#pragma once
#include "config.h"
#ifdef USE_WIFI_MODE
#include <WiFi.h>

class WiFiManager {
public:
  void init();
  void update();
private:
  WiFiServer server = WiFiServer(WIFI_PORT);
  WiFiClient client;
  bool alreadyConnected = false;
};

extern WiFiManager wifiManager;
#endif
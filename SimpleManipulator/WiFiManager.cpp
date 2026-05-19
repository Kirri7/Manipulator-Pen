#include "WiFiManager.h"

#ifdef USE_WIFI_MODE
WiFiManager wifiManager;

void WiFiManager::init() {
  Serial.print("Connecting to ");
  // Serial.println(WiFi.SSID());
  Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(500);
  }
  server.begin();
  digitalWrite(LED_BUILTIN, HIGH);
  
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  
  Serial.print("WiFi connected, listening on port ");
  Serial.println(WIFI_PORT);
}

void WiFiManager::update() {
  if (!client) {
      client = server.available();
      digitalWrite(LED_BUILTIN, LOW);
  }
  if (client) {
    if (!alreadyConnected) {
        client.flush();
        alreadyConnected = true;
        digitalWrite(LED_BUILTIN, HIGH);
    }
    if (client.available() > 0) {
        //str = client.readStringUntil('\n');  // read entire response
        // TODO DRY
        int32_t value;
        if (client.available() == sizeof(value)) {
            client.readBytes((char*)&value, sizeof(value));
            g_Input.right = (value & (1 << 0)) != 0;
            g_Input.left  = (value & (1 << 8)) != 0;
            g_Input.up    = (value & (1 << 16)) != 0;
            g_Input.down  = (value & (1 << 24)) != 0;
        } else {
            while (client.available()) {};  // discard corrupt packet
        }
    }
  }
}
#endif
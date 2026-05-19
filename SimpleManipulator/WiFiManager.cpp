#include "WiFiManager.h"
#include "PacketParser.h"

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
    if (client.available() >= sizeof(int32_t) {
        uint8_t buffer[4];
        client.readBytes(buffer, 4);
        PacketParser::parseCommand(buffer, 4);
    }
  }
}
#endif
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
  if (client && client.connected())
  {
    digitalWrite(LED_BUILTIN, HIGH);
    
    if (client.available() >= sizeof(int32_t)) {
        uint8_t buffer[4];
        client.readBytes(buffer, 4);
        PacketParser::parseAngles(buffer, 4);
    }
    // If available() < 4, we just wait for the next loop. 
    // No flushing, no blocking.
  }
  else
  {
      if (client) client.stop(); // Clean up old connection
      // while(client.available()) client.read();
      digitalWrite(LED_BUILTIN, LOW);
      WiFiClient newClient = server.available();

      if (newClient.connected()) {
          client = newClient;
          client.setTimeout(0); // Set non-blocking for future reads
          Serial.println("New client connected!");
      }
  }
  // If no new client, do nothing. We wait for next update().
}
#endif
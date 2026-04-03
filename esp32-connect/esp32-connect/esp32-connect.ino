// ESP32 Wi-Fi Receiver Module

#include <Arduino.h>
#include <WiFi.h>
#define LED_BUILTIN 2
#define serverPort 10000
#define ledPin 2

const char* ssid = "xxx";
const char* password = "xxx";


struct ReceivedPhoneData {
    bool left_signal;
    bool right_signal;
    bool up_signal;
    bool down_signal;
};

class WiFiReceiver {
private:
    WiFiServer server;
    bool log_to_serial;
    bool connected;
    int16_t seqExpected;

public:
    WiFiReceiver(const bool& do_logging) : connected(false), seqExpected(0), log_to_serial(do_logging), server(serverPort) {}
    
    void begin() {
        printlnSerial("Connecting to " + String(ssid));
        printlnSerial("Attempting to connect to WiFi network SSID " + String(ssid) + " password " + String(password));
        
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            printSerial(".");
            delay(500);
        }
        
        server.begin();
        printWifiStatus();
        printlnSerial("Listening on port " + String(serverPort));
        digitalWrite(ledPin, HIGH);
        delay(20);
        digitalWrite(ledPin, LOW);
    }
    
    ReceivedPhoneData receive() {
        if (!WiFi.isConnected()) {
            connected = false;
            digitalWrite(ledPin, LOW);
            printlnSerial("WiFiReceiver not initialized. Call begin() first.");
            return {false, false, false, false};
        }
        
        static WiFiClient client;

        if (!client) {
            connected = false;
            digitalWrite(LED_BUILTIN, LOW);
            client = server.available(); // Check for incoming clients
        }
        if (client) {
            if (!connected) {
                client.flush();
                connected = true;
                digitalWrite(ledPin, HIGH);
                printlnSerial("Client connected!");
            }

            int length = client.available();
            if (length > 0) {
                if (length == sizeof(int32_t)) {
                    int32_t value;
                    client.readBytes((char*)&value, sizeof(value));
                    ReceivedPhoneData data;
                    data.left_signal = (value & (1 << 0)) != 0;
                    data.right_signal = (value & (1 << 8)) != 0;
                    data.up_signal = (value & (1 << 16)) != 0;
                    data.down_signal = (value & (1 << 24)) != 0;
                    printlnSerial(
                        "value " + String(value) +
                        ",left " + String(data.left_signal) +
                        ",right " + String(data.right_signal) +
                        ",up " + String(data.up_signal) +
                        ",down " + String(data.down_signal)
                    );
                    return data;
                } else {
                    printSerial("Received invalid packet length: ");
                    while (client.available()) printSerial(String((char)client.read()));
                    printlnSerial("");
                }
            }
            else {
                // Пустые пакеты при выходе из приложения или в перерыве между отправкой данных
            }
        }
        return {false, false, false, false}; // no valid data received -> no movement
    }
    
    void printWifiStatus() {
        if (!log_to_serial) return;

        Serial.print("\nSSID: ");
        Serial.println(WiFi.SSID());
        
        IPAddress ip = WiFi.localIP();
        Serial.print("IP Address: ");
        Serial.println(ip);
        
        long rssi = WiFi.RSSI();
        Serial.print("signal strength (RSSI):");
        Serial.print(rssi);
        Serial.println(" dBm");
    }

    size_t printlnSerial(const String& message) {
        if (!log_to_serial) return 0;
        return Serial.println(message);
    }

    size_t printSerial(const String& message) {
        if (!log_to_serial) return 0;
        return Serial.print(message);
    }
};


WiFiReceiver receiver(true);

void setup() {
    pinMode(ledPin, OUTPUT);
    Serial.begin(2000000);

    receiver.begin();
}

void loop() {
    ReceivedPhoneData data = receiver.receive();
    
    if (data.left_signal) {
        Serial.println("Left button pressed");
    }
    if (data.right_signal) {
        Serial.println("Right button pressed");
    }
    if (data.up_signal) {
        Serial.println("Up button pressed");
    }
    if (data.down_signal) {
        Serial.println("Down button pressed");
    }
    
    delay(200);
}
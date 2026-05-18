#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Service and Characteristic UUIDsadfasdf
#define SERVICE_UUID        "acc0a4a9-f284-4eac-8fa5-d825c55ce64c"
#define CHARACTERISTIC_UUID "fc18c54c-2f23-4c05-84bd-338ca880b786"

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;

// Simulated sensor data
int counter = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Peripheral Device!");

  // Initialize BLE Device with a name
  BLEDevice::init("ESP32-Sensor-Device");

  // Create BLE Server
  pServer = BLEDevice::createServer();
  Serial.println("Server created");

  // Create Service
  pService = pServer->createService(SERVICE_UUID);
  Serial.println("Service created");

  // Create Characteristic with READ and NOTIFY properties
  // READ: allows laptop to read the value
  // NOTIFY: allows device to push updates to laptop
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  Serial.println("Characteristic created");

  // Set initial value
  pCharacteristic->setValue("Ready");
  
  // Start the service
  pService->start();
  Serial.println("Service started");

  // Configure advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  
  // Start advertising to make device discoverable
  BLEDevice::startAdvertising();
  Serial.println("BLE Peripheral is now discoverable!");
  Serial.println("Device Name: ESP32-Sensor-Device");
}

void loop() {
  // Simulate sensor data updates
  counter++;
  
  // Create a JSON-like string with sensor data
  String sensorData = "{\"temp\":" + String(20 + (counter % 10)) + 
                      ",\"humidity\":" + String(40 + (counter % 20)) + 
                      ",\"battery\":" + String(100 - (counter % 20)) + 
                      ",\"counter\":" + String(counter) + "}";
  
  // Update the characteristic value
  pCharacteristic->setValue(sensorData.c_str());
  
  // Notify all connected clients about the new value
  pCharacteristic->notify();
  
  // Print to serial for debugging
  Serial.print("Sending data: ");
  Serial.println(sensorData.c_str());
  
  // Wait 2 seconds before next update
  delay(2000);
}

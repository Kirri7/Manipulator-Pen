#include "BLEManager.h"

#ifdef USE_BLE_MODE
BLEManager bleManager;
BLEUUID BLEManager::serviceUUID(SERVICE_UUID);
BLEUUID BLEManager::charUUID(CHARACTERISTIC_UUID);
BLEManager* BLEManager::MyClientCallback::selfPtr = nullptr;
BLEManager* BLEManager::MyAdvertisedDeviceCallbacks::selfPtr = nullptr;

void BLEManager::init() {
  Serial.println("Starting BLE Client application...");
  BLEDevice::init("SimpleManipulator-BLE-Client");
  
  myDevice = nullptr;
  
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void BLEManager::update() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("Connected to BLE Server.");
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      Serial.println("Failed to connect.");
    }
    doConnect = false;
  }
  else if (doConnect == false && connected == false) {
    // Restart scanning periodically if needed
    static uint32_t lastScan = 0;
    if (millis() - lastScan > 5000) {
        Serial.println("Scanning again...");
        BLEDevice::getScan()->start(5, false);
        lastScan = millis();
    }
  }
}

bool BLEManager::connectToServer() {
  if (myDevice == nullptr) return false;
  
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  
  if(pClient->connect(myDevice)) {
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService) {
      pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
      // if(pRemoteCharacteristic->canRead())
      if (pRemoteCharacteristic && pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->registerForNotify(BLEManager::notifyCallback);
        connected = true;
        return true;
      }
    }
  }
  return false;
}

void BLEManager::notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                    uint8_t* pData, size_t length, bool isNotify) {
    // TODO unify packet recieving logic for all inputs
    // The sender sends an int32_t, which is 4 bytes.
    if (length != sizeof(int32_t)) {
        // Optional: Log error or ignore invalid packet
        return;
    }
    uint32_t command = 0;
    memcpy(&command, pData, length);
    // Bit 0: Right
    g_Input.right = ((command & (1 << 0)) != 0);
    
    // Bit 8: Left
    g_Input.left =  ((command & (1 << 8)) != 0);
    
    // Bit 16: Up
    g_Input.up =    ((command & (1 << 16)) != 0);
    
    // Bit 24: Down
    g_Input.down =  ((command & (1 << 24)) != 0);
    // Serial.printf("Cmd: 0x%08X | R:%d L:%d U:%d D:%d\n", command, g_Input.right, g_Input.left, g_Input.up, g_Input.down);
}

void BLEManager::MyAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice adv) {
  if (adv.haveServiceUUID() && adv.isAdvertisingService(serviceUUID)) {
    BLEDevice::getScan()->stop();
    bleManager.myDevice = new BLEAdvertisedDevice(adv);
    bleManager.doConnect = true;
    // doScan = true;
  }
}

void BLEManager::MyClientCallback::onConnect(BLEClient* pclient) {
    bleManager.connected = true;
    Serial.println("BLE Connected");
}

void BLEManager::MyClientCallback::onDisconnect(BLEClient* pclient) {
    bleManager.connected = false;
    Serial.println("BLE Disconnected");
}
#endif
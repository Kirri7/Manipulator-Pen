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
  // Parse JSON (simplified from original)
  // Since we can't easily pass 'this', we assume pData contains JSON string
  String jsonData((char*)pData);
  int yawIdx = jsonData.indexOf("\"yaw\"");
  int pitchIdx = jsonData.indexOf("\"pitch\"");
  
  if (yawIdx >= 0 && pitchIdx >= 0) {
    int colonIdx = jsonData.indexOf(':', yawIdx);
    int commaIdx = jsonData.indexOf(',', colonIdx);
    String yawStr = jsonData.substring(colonIdx + 1, commaIdx);
    float yaw = yawStr.toFloat();
    
    colonIdx = jsonData.indexOf(':', pitchIdx);
    commaIdx = jsonData.indexOf(',', colonIdx);
    String pitchStr = jsonData.substring(colonIdx + 1, commaIdx);
    float pitch = pitchStr.toFloat();
    
    const float THRESH = 15.0f;
    g_Input.right = (yaw > THRESH);
    g_Input.left  = (yaw < -THRESH);
    g_Input.up    = (pitch > THRESH);
    g_Input.down  = (pitch < -THRESH);
  }
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
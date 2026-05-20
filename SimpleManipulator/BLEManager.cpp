#include "BLEManager.h"
#include "PacketParser.h"
// TODO do code review

#ifdef USE_BLE_MODE
BLEManager bleManager;
BLEUUID BLEManager::remoteSUUID(REMOTE_SERVICE_UUID);
BLEUUID BLEManager::remoteCUUID(REMOTE_CHARACTERISTIC_UUID);
BLEUUID BLEManager::computerSUUID(COMPUTER_SERVICE_UUID);
BLEUUID BLEManager::computerCUUID(COMPUTER_CHARACTERISTIC_UUID);
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
  // подключение к нужному устройству из списка
  if (!myDevice || !currentServiceUUID || !currentCharUUID) return false;
  
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  
  if(pClient->connect(myDevice)) {
    BLERemoteService* pRemoteService = pClient->getService(*currentServiceUUID);
    if (pRemoteService) {
      pRemoteCharacteristic = pRemoteService->getCharacteristic(*currentCharUUID);
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
    PacketParser::parseCommand(pData, length);
    // Serial.printf("Cmd: 0x%08X | R:%d L:%d U:%d D:%d\n", command, g_Input.right, g_Input.left, g_Input.up, g_Input.down);
}

void BLEManager::MyAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice adv) {
  // Скаинирование каждого нового устройства в надежде найти нужное
  if (adv.haveServiceUUID()) {
    if (adv.isAdvertisingService(computerSUUID)) {
        bleManager.currentServiceUUID = &computerSUUID;
        bleManager.currentCharUUID = &computerCUUID;
    } else if (adv.isAdvertisingService(remoteSUUID)) {
        bleManager.currentServiceUUID = &remoteSUUID;
        bleManager.currentCharUUID = &remoteCUUID;
    } else {
        return;
    }
    BLEDevice::getScan()->stop();
    bleManager.myDevice = new BLEAdvertisedDevice(adv); // TODO think about delete
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
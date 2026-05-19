#pragma once
#include "config.h"
#ifdef USE_BLE_MODE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>

class BLEManager {
public:
  void init();
  void update();
  // Expose callback registration for external use if needed, but internal here
private:
  BLEClient* pClient;
  BLERemoteCharacteristic* pRemoteCharacteristic;
  BLEAdvertisedDevice* myDevice;
  boolean doConnect = false;
  boolean connected = false;
  
  static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                      uint8_t* pData, size_t length, bool isNotify);
  bool connectToServer();
  
  class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient);
    
    void onDisconnect(BLEClient* pclient);

    static BLEManager* selfPtr;
    friend class BLEManager;
  };

  class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice);
    static BLEManager* selfPtr;
    friend class BLEManager;
  };
};

extern BLEManager bleManager;
#endif
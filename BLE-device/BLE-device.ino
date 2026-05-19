#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// I2Cdev and MPU6050 libraries
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

// Service and Characteristic UUIDs
#define SERVICE_UUID        "acc0a4a9-f284-4eac-8fa5-d825c55ce64c"
#define CHARACTERISTIC_UUID "fc18c54c-2f23-4c05-84bd-338ca880b786"

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;

// MPU6050 instance
MPU6050 mpu;

// MPU control/status vars
bool dmpReady = false;
uint8_t mpuIntStatus;
uint8_t devStatus;
uint16_t packetSize;
uint16_t fifoCount;
uint8_t fifoBuffer[64];

// orientation/motion vars
Quaternion q;
VectorInt16 aa;
VectorFloat gravity;
float ypr[3];  // yaw, pitch, roll

#define INTERRUPT_PIN 15  // MPU INT pin connected to GPIO15 on ESP32

volatile bool mpuInterrupt = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Peripheral Device with MPU6050!");

  // Initialize MPU6050
  mpu_initialize();

  // Initialize BLE Device with a name
  BLEDevice::init("ESP32-MPU6050-BLE");

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
  Serial.println("Device Name: ESP32-MPU6050-BLE");
}

// MPU6050 interrupt handler
void ICACHE_RAM_ATTR dmpDataReady() {
    mpuInterrupt = true;
}

// MPU6050 initialization
void mpu_initialize() {
  // join I2C bus
  Wire.begin();
  Wire.setClock(400000); // 400kHz I2C clock

  // initialize device
  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT);

  // verify connection
  Serial.println(F("Testing device connections..."));
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  // load and configure the DMP
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // supply gyro offsets (you may need to calibrate these for your specific sensor)
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setZAccelOffset(0);

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // turn on the DMP
    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    Serial.println(F("Enabling interrupt detection..."));
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;

    // get expected DMP packet size
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    // ERROR!
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }
}

#define YAW_THRESHOLD     45.0f   // yaw threshold
#define PITCH_THRESHOLD   45.0f   // pitch threshold

uint32_t buildManipulatorCommand(float* ypr) {
    uint32_t command = 0;
    float yawDegrees = ypr[0] * 180/M_PI;
    float pitchDegrees = ypr[1] * 180/M_PI;
    // Right
    if (yawDegrees > YAW_THRESHOLD) command |= (1u << 0);
    // Left
    if (yawDegrees < -YAW_THRESHOLD) command |= (1u << 8);
    // Up
    if (pitchDegrees > PITCH_THRESHOLD) command |= (1u << 16);
    // Down
    if (pitchDegrees < -PITCH_THRESHOLD) command |= (1u << 24);
    return command;
}

void loop() {
  // Read MPU6050 data if DMP is ready
  if (dmpReady) {
    // wait for MPU interrupt or extra packet(s) available
    if (!mpuInterrupt && fifoCount < packetSize) {
      delay(10);
      return;
    }

    // reset interrupt flag and get INT_STATUS byte
    mpuInterrupt = false;
    mpuIntStatus = mpu.getIntStatus();

    // get current FIFO count
    fifoCount = mpu.getFIFOCount();

    // check for overflow
    if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
      mpu.resetFIFO();
      Serial.println(F("FIFO overflow!"));
    } else if (mpuIntStatus & 0x02) {
      // wait for correct available data length
      while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

      // read a packet from FIFO
      mpu.getFIFOBytes(fifoBuffer, packetSize);

      // track FIFO count
      fifoCount -= packetSize;

      // Get quaternion and calculate yaw/pitch/roll
      mpu.dmpGetQuaternion(&q, fifoBuffer);
      mpu.dmpGetGravity(&gravity, &q);
      mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

      // Convert to degrees
      float yaw = ypr[0] * 180 / M_PI;
      float pitch = ypr[1] * 180 / M_PI;
      float roll = ypr[2] * 180 / M_PI;

      // Create JSON string with real MPU6050 data
      String sensorData = "{\"yaw\":" + String(yaw, 2) + 
                          ",\"pitch\":" + String(pitch, 2) + 
                          ",\"roll\":" + String(roll, 2) + 
                          ",\"quat_w\":" + String(q.w, 4) + 
                          ",\"quat_x\":" + String(q.x, 4) + 
                          ",\"quat_y\":" + String(q.y, 4) + 
                          ",\"quat_z\":" + String(q.z, 4) + "}";
      
      // Update the characteristic value
      // pCharacteristic->setValue(sensorData.c_str());
      // Notify all connected clients about the new value
      // pCharacteristic->notify();
      // Send manipulator command as BLE characteristic (left,right,up,down bits)
      float yprArray[3] = {ypr[0], ypr[1], ypr[2]};
      uint32_t cmd = buildManipulatorCommand(yprArray);
      // Set the command as 4-byte little-endian value
      pCharacteristic->setValue((uint8_t*)&cmd, sizeof(cmd));
      pCharacteristic->notify();
      // Print to serial for debugging
      Serial.print("Sending MPU data: ");
      Serial.println(sensorData.c_str());
    }
  } else {
    // If DMP is not ready, send error status
    String errorData = "{\"error\":\"MPU6050 not ready\"}";
    pCharacteristic->setValue(errorData.c_str());
    pCharacteristic->notify();
    delay(1000);
  }
}

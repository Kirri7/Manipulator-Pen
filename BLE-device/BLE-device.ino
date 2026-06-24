#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <cstdint>
#include <array>
#include <vector>

// Libraries
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

#define SERVICE_UUID        "acc0a4a9-f284-4eac-8fa5-d825c55ce64c"
#define CHARACTERISTIC_UUID "fc18c54c-2f23-4c05-84bd-338ca880b786"
#define FEEDBACK_UUID       "58a003c5-9ac0-4e1e-a15b-cdca9fcafec1"

#define INTERRUPT_PIN 4  // MPU INT pin connected to GPIO15 on ESP32
volatile bool mpuInterrupt = false;

void ICACHE_RAM_ATTR dmpDataReady() {
    mpuInterrupt = true;
}

// --- Data Structures ---

struct SensorData {
    Quaternion q;
    float ypr[3];
    bool isValid = false;
};

struct Packet {
    std::vector<uint8_t> data;
};

// --- Pipeline Modules ---

/**
 * 1. Сбор данных (Data Acquisition)
 */
class SensorCollector {
private:
    MPU6050 mpu;
    uint8_t fifoBuffer[64];
    uint16_t packetSize;
    bool dmpReady = false;

public:
    bool begin() { // mpu_init
        Wire.begin();
        Wire.setClock(400000); // 400kHz I2C clock
        Serial.println(F("Initializing I2C devices..."));
        mpu.initialize();
        Serial.println(F("Initializing DMP..."));
        uint8_t devStatus = mpu.dmpInitialize();

        // Gyro offsets
        mpu.setXGyroOffset(0);
        mpu.setYGyroOffset(0);
        mpu.setZGyroOffset(0);
        mpu.setZAccelOffset(0);

        if (devStatus == 0) {
            Serial.println(F("Enabling DMP..."));
            mpu.setDMPEnabled(true);

            Serial.println(F("Enabling interrupt detection..."));
            attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
            uint8_t mpuIntStatus = mpu.getIntStatus();

            Serial.println(F("DMP ready! Waiting for first interrupt..."));
            dmpReady = true;
            packetSize = mpu.dmpGetFIFOPacketSize();
            return true;
        }
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
        return false;
    }

    void calibrate(Quaternion& qCal) {
        Serial.println("DMP stabilization (~2 sec)...");
        mpu.resetFIFO();
        delay(2000); // даём время фильтру в DMP стабилизироваться
        Serial.println("Calibrating zero position. DO NOT MOVE...");
        Quaternion q;
        float sumW = 0, sumX = 0, sumY = 0, sumZ = 0;
        int count = 0;
        const int need = 100;
        unsigned long start = millis();
        mpu.resetFIFO(); // чистим перед стартом
        while (count < need && millis() - start < 3000) {
          if (mpuInterrupt || mpu.getFIFOCount() >= packetSize) {
            mpuInterrupt = false;
            // Если накопилось много пакетов — сбрасываем старые, читаем только свежий
            while (mpu.getFIFOCount() >= packetSize * 2) {
              mpu.getFIFOBytes(fifoBuffer, packetSize);
            }
            mpu.getFIFOBytes(fifoBuffer, packetSize);
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            sumW += q.w; sumX += q.x; sumY += q.y; sumZ += q.z;
            count++;
            if (count % 20 == 0) Serial.print(".");
          }
          yield(); // не блокируем WiFi/BLE на ESP32
        }
        if (count > 0) {
          // Усредняем и нормализуем
          q.w = sumW / count; q.x = sumX / count; q.y = sumY / count; q.z = sumZ / count;
          float norm = sqrt(q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
          q.w /= norm; q.x /= norm; q.y /= norm; q.z /= norm;
          // Сохраняем ОБРАТНЫЙ (сопряжённый) кватернион: qCal = q^{-1}
          // Для единичного кватерниона: q^{-1} = (w, -x, -y, -z)
          qCal.w =  q.w;
          qCal.x = -q.x;
          qCal.y = -q.y;
          qCal.z = -q.z;
          Serial.println("\nCalibration done. qCal stored.");
        } else {
          Serial.println("\nCalibration FAILED!");
        }
    }

    bool collect(SensorData& outData) {
        if (!dmpReady) return false;
        uint16_t fifoCount = mpu.getFIFOCount();

        if (!mpuInterrupt && fifoCount < packetSize) {
          delay(10);
          return false;
        }

        mpuInterrupt = false;
        uint8_t mpuIntStatus = mpu.getIntStatus();

        if (fifoCount > packetSize * 2) {
            mpu.resetFIFO();
            return false; 
        }
        if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
          mpu.resetFIFO();
          // TODO think about it
          // Serial.println(F("FIFO overflow!"));
          return false;
        }
        if (!(mpuIntStatus & 0x02)) return false;

        while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

        mpu.getFIFOBytes(fifoBuffer, packetSize);

        fifoCount -= packetSize;

        mpu.dmpGetQuaternion(&outData.q, fifoBuffer);
        // mpu.dmpGetYawPitchRoll
        outData.isValid = true;
        return true;
    }
};

/**
 * 2. Фильтрация (Sensor Fusion)
 */
class SensorFusion {
public:
    void process(SensorData& data, const Quaternion& qCal) {
        // Здесь происходит применение qCal для получения относительных углов
        pass_fusion_logic(data, qCal);
    }

private:
    void pass_fusion_logic(SensorData& data, const Quaternion& qCal) {
        // Вычисляем относительный кватернион: qRel = qCal * q_current
        // qCal уже хранит обратный (conjugate) от калибровочного момента
        Quaternion qRel;
        Quaternion q = data.q;
        qRel.w = qCal.w*q.w - qCal.x*q.x - qCal.y*q.y - qCal.z*q.z;
        qRel.x = qCal.w*q.x + qCal.x*q.w + qCal.y*q.z - qCal.z*q.y;
        qRel.y = qCal.w*q.y - qCal.x*q.z + qCal.y*q.w + qCal.z*q.x;
        qRel.z = qCal.w*q.z + qCal.x*q.y - qCal.y*q.x + qCal.z*q.w;
        // Из относительного кватерниона получаем углы
        // mpu.dmpGetGravity(&gravity, &qRel);
        // mpu.dmpGetYawPitchRoll(ypr, &qRel, &gravity);
        data.q = qRel;
    }
};

/**
 * 3. Усреднение (Moving Average)
 */
class MovingAverage {
private:
    static const int WINDOW_SIZE = 5;
    std::vector<SensorData> window;

public:
    void add(const SensorData& data) {
        if (window.size() >= WINDOW_SIZE) window.erase(window.begin());
        window.push_back(data);
    }

    SensorData getAverage() {
        if (window.empty()) return SensorData();

        SensorData avg;
        float sw = 0, sx = 0, sy = 0, sz = 0;
        for (const auto& d : window) {
            sw += d.q.w; sx += d.q.x; sy += d.q.y; sz += d.q.z;
        }
        avg.q.w = sw / window.size();
        avg.q.x = sx / window.size();
        avg.q.y = sy / window.size();
        avg.q.z = sz / window.size();
        avg.isValid = true;
        return avg;
    }
};

/**
 * 4. Сериализация (Serialization)
 */
class Serializer {
public:
    Packet pack(const SensorData& data) {
        String sensorData = "{\"yaw\":" + String(0, 2) +
                            ",\"pitch\":" + String(0, 2) +
                            ",\"roll\":" + String(0, 2) +
                            ",\"quat_w\":" + String(data.q.w, 4) +
                            ",\"quat_x\":" + String(data.q.x, 4) +
                            ",\"quat_y\":" + String(data.q.y, 4) +
                            ",\"quat_z\":" + String(data.q.z, 4) + "}";
        Serial.print("Sending MPU data: ");
        Serial.println(sensorData.c_str());

        Packet p;
        // Упаковка в 16 байт (4 float)
        struct QuatPacket { float w, x, y, z; } pkt;
        pkt.w = data.q.w; pkt.x = data.q.x; pkt.y = data.q.y; pkt.z = data.q.z;

        uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&pkt);
        p.data.assign(bytePtr, bytePtr + sizeof(pkt));
        return p;
    }
};

/**
 * 5. Передача (Wireless Transmission)
 */
class BLETransmitter {
private:
    BLECharacteristic* pCharacteristic;
    BLECharacteristic* pFeedback;

public:
    void begin(BLECharacteristic* characteristic, BLECharacteristic* feedback) {
        pCharacteristic = characteristic;
        pFeedback = feedback;
    }

    void transmit(const Packet& packet) {
        pCharacteristic->setValue(packet.data.data(), packet.data.size());
        pCharacteristic->notify();
    }
};

/**
 * Асинхронная обработка обратной связи (Feedback Handler)
 */
class FeedbackHandler {
public:
    // Эта функция должна запускаться в отдельной задаче (FreeRTOS Task)
    void asyncProcess() {
        // Ожидание входящих данных через callback или опрос
        // В ESP32 это делается через BLE Characteristic Callbacks
    }
};

// --- Main Application Controller ---

class Controller {
private:
    SensorCollector collector;
    SensorFusion fusion;
    MovingAverage averager;
    Serializer serializer;
    BLETransmitter transmitter;
    FeedbackHandler feedback;

    Quaternion qCal;
    BLECharacteristic* pChar;
    BLECharacteristic* pFeedb;

public:
    void setup(BLECharacteristic* characteristic, BLECharacteristic* feedback) {
        pChar = characteristic;
        pFeedb = feedback;
        transmitter.begin(pChar, feedback);

        if (!collector.begin()) {
            Serial.println("Sensor init failed!");
            return;
        }

        collector.calibrate(qCal);
        Serial.println("Pipeline Ready.");
    }

    void run() {
        static unsigned long lastRun = 0;
        const unsigned long interval = 20; // 20ms = 50Hz.
        unsigned long now = millis();
        if (now - lastRun < interval) return;
        lastRun = now;

        SensorData rawData;

        // 1. Сбор
        if (collector.collect(rawData)) // get Quaternion
        {
            // 2. Фильтрация
            fusion.process(rawData, qCal); // qCal treatment

            // 3. Усреднение
            averager.add(rawData);
            SensorData smoothData = averager.getAverage();

            // 4. Сериализация
            Packet packet = serializer.pack(smoothData); // pack Quaternion

            // 5. Передача
            transmitter.transmit(packet); // send packet
        }
    }

    void handleFeedback() {
        feedback.asyncProcess();
    }
};

// --- Global Objects & Boilerplate ---

Controller app;
BLECharacteristic* globalChar;
BLECharacteristic* globalFeedb;

// BLE Callback для асинхронной обратной связи
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pFeedback) {
        Serial.println("Feedback received!");
    }
};

void setup() {
    Serial.begin(115200);

    // BLE Setup
    BLEDevice::init("ESP32-MPU6050-BLE");
    BLEServer *pServer = BLEDevice::createServer();
    Serial.println("Server created");

    BLEService *pService = pServer->createService(SERVICE_UUID);
    Serial.println("Service created");

    globalChar = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    globalChar->setValue("Ready");
    Serial.println("Characteristic created");

    globalFeedb = pService->createCharacteristic(
        FEEDBACK_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_WRITE
    );
    globalFeedb->setValue("Ready");

    globalFeedb->setCallbacks(new MyCallbacks()); // ???
    pService->start();
    Serial.println("Service started");

    // BLEDevice::getAdvertising()->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);

    BLEDevice::startAdvertising();
    Serial.println("BLE Peripheral is now discoverable!");
    Serial.println("Device Name: ESP32-MPU6050-BLE");

    // Start App
    // 1) mpu_init
    // 2) calibrate
    app.setup(globalChar, globalFeedb);
}

void loop() {
    app.run();
    // В ESP32 loop() не должен блокироваться,
    // чтобы работали системные задачи Wi-Fi/BLE
    delay(10);
}

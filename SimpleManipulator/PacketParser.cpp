#include "PacketParser.h"
#include "config.h"
#include <endian.h>

bool PacketParser::parseCommand(const uint8_t* pData, size_t length)  {
    if (length != sizeof(int32_t) or !pData) {
        return false;
    }

    uint32_t command = 0;
    // Используем memcpy для безопасного копирования (избегаем проблем с alignment)
    memcpy(&command, pData, sizeof(uint32_t));

    // Bit 0: Right
    g_Input.right = ((command & (1 << 0)) != 0);
    // Bit 8: Left
    g_Input.left  = ((command & (1 << 8)) != 0);
    // Bit 16: Up
    g_Input.up    = ((command & (1 << 16)) != 0);
    // Bit 24: Down
    g_Input.down  = ((command & (1 << 24)) != 0);

    return true;
}

/*
bool PacketParser::parseAngles(const uint8_t* pData, size_t length) {
    if (length != 6) return false;
    AnglesPacket packet;
    memcpy(&packet, pData, sizeof(packet));
    
    // Для BLE обычно little-endian или be16toh() для big-endian
    packet.yaw = le16toh(packet.yaw);   
    packet.pitch = le16toh(packet.pitch);
    packet.roll = le16toh(packet.roll);
    
    constexpr float ANGLE_SCALE = 100.0f;
    
    noInterrupts();
    // g_TargetAngles.yaw = packet.yaw / ANGLE_SCALE;
    g_TargetAngles.pitch = packet.pitch / ANGLE_SCALE;
    g_TargetAngles.roll = packet.roll / ANGLE_SCALE;
    g_TargetAngles.newUpdate = true;
    interrupts();

    // Serial.print("Parsed angles: ");
    // Serial.print(g_TargetAngles.pitch);
    // Serial.print(", ");
    // Serial.println(g_TargetAngles.roll);
    
    return true;
}
*/

#include <cmath>

// Структура для удобного хранения результата
struct EulerAngles {
    float yaw;
    float pitch;
    float roll;
};

EulerAngles quaternionToEuler(float w, float x, float y, float z) {
    EulerAngles angles;

    // Roll (x-axis rotation)
    float sinr_cosp = 2 * (w * x + y * z);
    float cosr_cosp = 1 - 2 * (x * x + y * y);
    angles.roll = std::atan2(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2 * (w * y - z * x);
    if (std::abs(sinp) >= 1)
        angles.pitch = std::copysign(M_PI / 2, sinp); // use 90 degrees if out of range
    else
        angles.pitch = std::asin(sinp);

    // Yaw (z-axis rotation)
    float siny_cosp = 2 * (w * z + x * y);
    float cosy_cosp = 1 - 2 * (y * y + z * z);
    angles.yaw = std::atan2(siny_cosp, cosy_cosp);

    // Конвертируем радианы в градусы (если ваша система работает в градусах)
    angles.roll = angles.roll * 180.0 / M_PI;
    angles.pitch = angles.pitch * 180.0 / M_PI;
    angles.yaw = angles.yaw * 180.0 / M_PI;

    return angles;
}

bool PacketParser::parseAngles(const uint8_t* pData, size_t length) {
    // Теперь ожидаем 16 байт (4 float по 4 байта каждый)
    if (length != 16) return false;

    // Структура для распаковки данных
    struct QuatPacket {
        float w, x, y, z;
    } pkt;

    // Копируем данные из буфера в структуру
    memcpy(&pkt, pData, sizeof(pkt));

    // Конвертируем кватернион в углы
    EulerAngles angles = quaternionToEuler(pkt.w, pkt.x, pkt.y, pkt.z);

    noInterrupts();
    // Обновляем глобальные переменные
    // g_TargetAngles.yaw = angles.yaw;
    g_TargetAngles.pitch = angles.pitch;
    g_TargetAngles.roll = angles.roll;
    g_TargetAngles.newUpdate = true;
    interrupts();

    return true;
}

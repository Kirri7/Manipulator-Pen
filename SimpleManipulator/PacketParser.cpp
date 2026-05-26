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

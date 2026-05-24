#include "PacketParser.h"
#include "config.h"

bool PacketParser::parseCommand(const uint8_t* pData, size_t length)  {
    if (length != sizeof(int32_t)) {
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
    // TODO think about packet structure
    /*
    // Минимум "Y0R0\n", максимум — разумный буфер
    if (length < 3 || length > 63) return false;
    char buf[64];
    memcpy(buf, pData, length);
    buf[length] = '\0';
    // Ищем маркер 'Y', чтобы пережить возможный мусор в начале
    char* start = strchr(buf, 'Y');
    if (!start) return false;
    float yaw = 0.0f;
    float roll = 0.0f;
    // На ESP32 %f в sscanf работает. Если когда-то перейдёте на AVR — замените на strtof
    if (sscanf(start, "Y%fR%f", &yaw, &roll) == 2) {
        noInterrupts();
        g_TargetAngles.yaw = yaw;
        g_TargetAngles.roll = roll;
        g_TargetAngles.valid = true;
        interrupts();
        return true;
    }
    */
    return false;
}

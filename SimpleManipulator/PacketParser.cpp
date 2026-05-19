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

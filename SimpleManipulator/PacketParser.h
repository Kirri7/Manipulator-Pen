#pragma once
#include <stdint.h>
#include <string.h>

class PacketParser {
public:
    struct AnglesPacket {
        int16_t yaw;
        int16_t pitch;
        int16_t roll;
    };
    /**
     * @brief Парсит 4-байтовый пакет и обновляет g_Input
     * @param pData Указатель на данные
     * @param length Размер полученных данных
     * @return true если пакет корректен и обработан, false если ошибка
     */
    static bool parseCommand(const uint8_t* pData, size_t length);
    static bool parseAngles(const uint8_t* pData, size_t length);
};

import asyncio
import inspect
import logging
import sys
import threading
import struct
try:
    import pygame
    PYGAME_AVAILABLE = True
except ImportError:
    PYGAME_AVAILABLE = False
from typing import Any, Optional

from bleak import BleakScanner, BleakClient
from bless import (
    BlessServer,
    BlessGATTCharacteristic,
    GATTCharacteristicProperties,
    GATTAttributePermissions,
)

# TODO add connection sound
# TODO add inter-server communication

# =============================================================================
# Конфигурация
# =============================================================================

# --- Устройство-источник (ESP32 пульт) ---
TARGET_NAME = "ESP32-MPU6050-BLE"
REMOTE_SERVICE_UUID = "acc0a4a9-f284-4eac-8fa5-d825c55ce64c"
REMOTE_CHAR_UUID = "fc18c54c-2f23-4c05-84bd-338ca880b786"

# --- Это устройство-приёмник (к нему подключается манипулятор) ---
LOCAL_NAME = "BLE-Gateway"
LOCAL_SERVICE_UUID = "6938e8b6-77d8-44e4-ab9d-d27918908cb8"  # любой другой UUID
LOCAL_CHAR_UUID = "e869108c-f2db-4772-a6ba-380a0761ef24"

# =============================================================================
# Logging
# =============================================================================

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger(__name__)

# Отдельный логгер для углов
angle_logger = logging.getLogger("AngleLogger")
angle_logger.setLevel(logging.INFO)
fh = logging.FileHandler("angles.log")
# fh.setFormatter(logging.Formatter("%(asctime)s, %(message)s", datefmt="%Y-%m-%d %H:%M:%S"))
angle_logger.addHandler(fh)

# =============================================================================
# Основной класс
# =============================================================================

class BLEGateway:
    def __init__(self):
        # Защита данных, т.к. bless на Win32/macOS дергает read_request из другого потока
        self._data_lock = threading.Lock()
        self._latest_data: bytearray = bytearray(b"NO_DATA")

        self._server: Optional[BlessServer] = None
        self._running = True
        self._sound_initialized = False
        if PYGAME_AVAILABLE:
            try:
                pygame.mixer.init()
                self._sound_initialized = True
                logger.info("Audio system initialized (pygame)")
            except Exception as e:
                logger.warning(f"Failed to init pygame mixer: {e}")
                self._sound_initialized = False
        else:
            logger.warning("pygame not found, sound playback disabled")
        self._play_sound('sounds/calibration.mp3')
        self._play_sound('sounds/calibration.mp3')
        self._play_sound('sounds/calibration.mp3')
        self._play_sound('sounds/calibration.mp3')

    def _play_sound(self, filepath):
        if not self._sound_initialized:
            return
        
        def play():
            try:
                logger.info(f"Playing sound: {filepath}")
                pygame.mixer.music.load(filepath)
                pygame.mixer.music.play()
                while pygame.mixer.music.get_busy():
                    pygame.time.Clock().tick(10)
            except Exception as e:
                logger.error(f"Sound playback error: {e}")
        thread = threading.Thread(target=play, daemon=True)
        thread.start()
    # -------------------------------------------------------------------------
    # Bless Server — манипулятор подключается к этому устройству
    # -------------------------------------------------------------------------
    def _read_request(self, characteristic: BlessGATTCharacteristic, **kwargs) -> bytearray:
        """Отдаём текущий обработанный пакет манипулятору."""
        with self._data_lock:
            payload = bytearray(self._latest_data)
        logger.debug("Server READ %s -> %s", characteristic.uuid, payload.hex())
        return payload

    def _write_request(self, characteristic: BlessGATTCharacteristic, value: Any, **kwargs):
        # Для избежания непредвиннего BlessError из _server.write_request_func
        """Если манипулятор что-то напишет — просто логируем."""
        logger.debug("Server WRITE %s <- %s", characteristic.uuid, value)
        characteristic.value = value

    async def _start_server(self):
        logger.info("Запускаем локальный GATT сервер для манипулятора...")
        self._server = BlessServer(name=LOCAL_NAME)
        self._server.read_request_func = self._read_request
        self._server.write_request_func = self._write_request

        await self._server.add_new_service(LOCAL_SERVICE_UUID)

        flags = (
            GATTCharacteristicProperties.read
            | GATTCharacteristicProperties.notify
        )
        permissions = GATTAttributePermissions.readable

        await self._server.add_new_characteristic(
            LOCAL_SERVICE_UUID,
            LOCAL_CHAR_UUID,
            flags,
            self._latest_data,
            permissions,
        )
        await self._server.start()
        logger.info("Сервер активен. Манипулятор может подключаться.")

    async def _push_to_manipulator(self, data: bytearray):
        """Обновляем кеш и рассылаем BLE подписчикам (notify)."""
        with self._data_lock:
            self._latest_data = data

        if self._server is None:
            return

        try:
            # На Linux (BlueZ) этот метод — корутина, на Win/mac может быть обычной функцией.
            char = self._server.get_characteristic(LOCAL_CHAR_UUID)
            if char is None:
                logger.error("Характеристика не найдена")
                return

            char.value = data

            result = self._server.update_value(LOCAL_SERVICE_UUID, LOCAL_CHAR_UUID)
            if inspect.isawaitable(result):
                await result # type: ignore
            
            logger.debug("Манипулятор успешно уведомлен")
        except Exception as exc:
            logger.warning("Не удалось уведомить манипулятор: %s", exc)

    # -------------------------------------------------------------------------
    # Bleak Client — это устройство подключается к ESP32 пульту
    # -------------------------------------------------------------------------
    def _on_remote_notification(self, sender: int, data: bytearray):
        """Каждый раз, когда ESP32 шлёт уведомление — отрабатка здесь."""
        logger.info("Данные от пульта [%s]: %s (hex: %s)", sender, data, data.hex())

        angles = self._process(data)

        if angles:
            pitch, roll, yaw = angles
            angle_logger.info(f"{pitch:.2f}, {roll:.2f}, {yaw:.2f}")

        # Передаём в серверную часть без блокировки bleak-колбека
        asyncio.create_task(self._push_to_manipulator(data))

    @staticmethod
    def _process(raw: bytearray) -> Optional[tuple[float, float, float]]:
        """
        Парсит 6 байт: 3 значения uint16 (little-endian).
        Возвращает (pitch, roll, yaw) или None.
        """
        if len(raw) != 6:
            return None
        try:
            # '<' - little-endian, 'H' - unsigned short (2 bytes)
            # распаковываем 3 значения по 2 байта каждое
            unscaled_values = struct.unpack('<hhh', raw)
            
            yaw_raw, pitch_raw, roll_raw = unscaled_values
            
            ANGLE_SCALE = 100.0
            
            return (
                pitch_raw / ANGLE_SCALE,
                roll_raw / ANGLE_SCALE,
                yaw_raw / ANGLE_SCALE
            )
        except Exception as e:
            logger.error("Ошибка обработки углов: %s", e)
            return None

    async def _client_loop(self):
        """Бесконечный цикл: сканировать -> подключиться -> принимать -> переподключиться."""
        while self._running:
            try:
                logger.info("Сканирование эфира в поисках '%s'...", TARGET_NAME)
                # TODO also use REMOTE_SERVICE_UUID for filtering
                device = await BleakScanner.find_device_by_filter(
                    lambda d, ad: d.name == TARGET_NAME,
                    timeout=15.0,
                )

                if device is None:
                    logger.warning("Пульт не найден. Повтор через 5 сек...")
                    await asyncio.sleep(5)
                    continue

                logger.info("Найдено устройство: %s @ %s", device.name, device.address)
                disconnected_event = asyncio.Event()

                def on_disconnect(client: BleakClient):
                    logger.warning("Связь с пультом потеряна.")
                    disconnected_event.set()

                async with BleakClient(device, disconnected_callback=on_disconnect) as client:
                    logger.info("Подключено к пульту.")
                    self._play_sound('sounds/calibration.mp3')
                    await client.start_notify(REMOTE_CHAR_UUID, self._on_remote_notification)
                    logger.info("Подписка на характеристику оформлена. Ожидаю данные...")

                    # Спим, пока соединение живо
                    await disconnected_event.wait()

            except Exception as exc:
                logger.error("Ошибка клиента: %s", exc, exc_info=True)
                await asyncio.sleep(5)

    # -------------------------------------------------------------------------
    # Жизненный цикл приложения
    # -------------------------------------------------------------------------
    async def _keepalive(self):
        """Удерживает event loop от завершения."""
        while self._running:
            await asyncio.sleep(3600)

    async def run(self):
        try:
            # Крутим и сервер, и клиента одновременно
            await self._start_server()
            await asyncio.gather(self._client_loop(), self._keepalive())
        except asyncio.CancelledError:
            logger.info("Получен сигнал остановки...")
        finally:
            self._running = False
            if self._server:
                await self._server.stop()
            logger.info("Выключение завершено.")

def main():
    gateway = BLEGateway()
    try:
        asyncio.run(gateway.run())
    except KeyboardInterrupt:
        logger.info("Остановлено пользователем.")

if __name__ == "__main__":
    main()
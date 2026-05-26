import asyncio
import inspect
import logging
import math
import sys
import threading
import struct
from typing import Any, Optional
import socket

try:
    from playsound3 import playsound
    SOUND_AVAILABLE = True
except ImportError:
    SOUND_AVAILABLE = False

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
        self._sound_lock = threading.Lock()
        self._last_sound_time = 0
        if not SOUND_AVAILABLE:
            logger.warning("Библиотека playsound не найдена. Звук отключен. Выполните: pip install playsound==1.2.2")
        else:
            logger.info("Audio system ready (playsound)")
        
        self._udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.udp_target = ("127.0.0.1", 5005) 


    def _play_sound(self, filepath):
        if not SOUND_AVAILABLE:
            return
        
        import time
        now = time.time()
        
        # Debounce: не проигрываем чаще 2 раз в секунду
        if now - self._last_sound_time < 2.0:
            logger.debug("Звук проигрывался слишком недавно, игнорируем")
            return
        
        with self._sound_lock:
            self._last_sound_time = now
            
            def play():
                try:
                    logger.info(f"Playing sound: {filepath}")
                    playsound(filepath)
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
    @staticmethod
    def _process(raw: bytearray) -> Optional[tuple[float, float, float, float]]:
        """
        Парсит 16 байт: 4 значения float (little-endian) для кватерниона (w, x, y, z).
        Возвращает (w, x, y, z) или None.
        """
        if len(raw) != 16:
            return None
        try:
            # '<' - little-endian, 'f' - float (4 bytes)
            # распаковываем 4 значения по 4 байта каждое
            w, x, y, z = struct.unpack('<ffff', raw)
            
            # Нормализуем кватернион
            norm = math.sqrt(w*w + x*x + y*y + z*z)
            if norm > 0:
                w /= norm
                x /= norm
                y /= norm
                z /= norm
            
            return (w, x, y, z)
        except Exception as e:
            logger.error("Ошибка обработки кватерниона: %s", e)
            return None

    def _on_remote_notification(self, sender: int, data: bytearray):
        """Каждый раз, когда ESP32 шлёт уведомление — отрабатка здесь."""
        logger.info("Данные от пульта [%s]: %s (hex: %s)", sender, data, data.hex())

        quat = self._process(data)

        if quat:
            w, x, y, z = quat
            message = f"{w},{x},{y},{z}".encode()
            try:
                self._udp_socket.sendto(message, self.udp_target)
            except Exception as e:
                logger.error(f"UDP Send error: {e}")
            # Для лога конвертируем кватернион в углы (yaw, pitch, roll)
            # Используем формулу для конвертации кватерниона в Euler angles

            # Пишем в файл для Ursina программы в формате: w, x, y, z
            angle_logger.info(f"{w:.4f}, {x:.4f}, {y:.4f}, {z:.4f}")
            logger.info(f"Quat: {w:.4f}, {x:.4f}, {y:.4f}, {z:.4f}")
        else:
            logger.warning("Не удалось распарсить кватернион")

        # Передаём сырые данные в серверную часть (16 байт кватерниона)
        asyncio.create_task(self._push_to_manipulator(bytearray(data)))

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
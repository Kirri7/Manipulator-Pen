import asyncio
import inspect
import logging
import math
import socket
import struct
import threading
from typing import Any, Optional, Tuple

from bleak import BleakScanner, BleakClient
from bless import (
    BlessServer,
    BlessGATTCharacteristic,
    GATTCharacteristicProperties,
    GATTAttributePermissions,
)

try:
    from playsound3 import playsound
    SOUND_AVAILABLE = True
except ImportError:
    SOUND_AVAILABLE = False

# =============================================================================
# Configuration & Logging
# =============================================================================

LOG_ANGLES_TERMINAL = False
TARGET_NAME = "ESP32-MPU6050-BLE"
REMOTE_SERVICE_UUID = "acc0a4a9-f284-4eac-8fa5-d825c55ce64c"
REMOTE_CHAR_UUID = "fc18c54c-2f23-4c05-84bd-338ca880b786"
LOCAL_NAME = "BLE-Gateway"
LOCAL_SERVICE_UUID = "6938e8b6-77d8-44e4-ab9d-d27918908cb8"
LOCAL_CHAR_UUID = "e869108c-f2db-4772-a6ba-380a0761ef24"
LOCAL_FEEDBACK_UUID = "58a003c5-9ac0-4e1e-a15b-cdca9fcafec1"

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(name)s: %(message)s")
logger = logging.getLogger("Gateway")

# =============================================================================
# Pipeline Components (Stages)
# =============================================================================

class DataCollector:
    """Stage 1: Сбор сырых данных (BLE Client)"""
    def __init__(self, target_name: str):
        self.target_name = target_name

    async def collect(self, callback_fn):
        while True:
            logger.info(f"Searching for {self.target_name}...")
            device = await BleakScanner.find_device_by_filter(
                lambda d, ad: d.name == self.target_name, timeout=15.0
            )
            if not device:
                logger.warning("Пульт не найден. Повтор через 5 сек...")
                await asyncio.sleep(5)
                continue
            
            logger.info("Найдено устройство: %s @ %s", device.name, device.address)
            disconnected_event = asyncio.Event()
            async def on_disconnect(client): 
                logger.warning("Связь с пультом потеряна.")
                disconnected_event.set()

            async with BleakClient(device, disconnected_callback=on_disconnect) as client:
                logger.info("Подключено к пульту.")

                # Trigger calibration here
                # TODO play sound
                await self.calibrate() # TODO
                
                await client.start_notify(REMOTE_CHAR_UUID, callback_fn)
                logger.info("Подписка на характеристику оформлена. Ожидаются данные...")
                await disconnected_event.wait()

    async def calibrate(self):
        """Алгоритм калибровки при включении"""
        logger.info("Starting sensor calibration...")
        pass # TODO: Implement Gyro/Accel offset calibration

class SensorFusion:
    """Stage 2: Фильтрация (Sensor Fusion / IMU processing)"""
    def process(self, raw_data: bytearray) -> Optional[Tuple[float, float, float, float]]:
        if len(raw_data) != 16:
            return None
        try:
            # '<' - little-endian, 'f' - float (4 bytes)
            # распаковываем 4 значения по 4 байта каждое
            w, x, y, z = struct.unpack('<ffff', raw_data)
            norm = math.sqrt(w*w + x*x + y*y + z*z)
            if norm > 0:
                w, x, y, z = w/norm, x/norm, y/norm, z/norm
            return (w, x, y, z)
        except Exception as e:
            logger.error(f"Fusion error: {e}")
            return None

class DataSmoother:
    """Stage 3: Усреднение (Sliding Window Average)"""
    def __init__(self, window_size: int = 5):
        self.window_size = window_size
        self.buffer = []

    def smooth(self, data: Tuple[float, float, float, float]) -> Tuple[float, float, float, float]:
        # TODO: Implement moving average logic
        pass 

class Serializer:
    """Stage 4: Сериализация (Упаковка в пакет)"""
    def serialize(self, quat: Tuple[float, float, float, float]) -> bytearray:
        return bytearray(struct.pack('<ffff', *quat))

class DataTransmitter:
    """Stage 5: Передача (Wireless / UDP / BLE Notify)"""
    def __init__(self, udp_target: Tuple[str, int]):
        self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.udp_target = udp_target

    def transmit_udp(self, quat: Tuple[float, float, float, float]):
        try:
            message = f"{quat[0]},{quat[1]},{quat[2]},{quat[3]}".encode()
            self.udp_socket.sendto(message, self.udp_target)
        except Exception as e:
            logger.error(f"UDP error: {e}")

    async def transmit_ble_notify(self, server: BlessServer, service_uuid: str, char_uuid: str, data: bytearray):
        """Передача через BLE Notify (_push_to_manipulator)"""
        try:
            char = server.get_characteristic(char_uuid)
            if char:
                char.value = data
                result = server.update_value(service_uuid, char_uuid)
                if inspect.isawaitable(result):
                    await result # type: ignore
                logger.debug("Манипулятор успешно уведомлен")
        except Exception as exc:
            logger.warning("Не удалось уведомить манипулятор: %s", exc)

class FeedbackHandler:
    """Обрабатывает команды, пришедшие от манипулятора через BLE Write"""
    async def handle_command(self, value: bytearray):
        if not value:
            return
        
        logger.info(f"FEEDBACK RECEIVED")
        # play sound
        return

# =============================================================================
# Main System Orchestrator
# =============================================================================

class BLEGateway:
    def __init__(self):
        self._running = True
        self._data_lock = threading.Lock()
        self._latest_data = bytearray(b"NO_DATA")
        self._server: Optional[BlessServer] = None
        
        # Sound system
        self._sound_lock = threading.Lock()
        self._last_sound_time = 0
        if not SOUND_AVAILABLE:
            logger.warning("Библиотека playsound не найдена. Звук отключен. Выполните: pip install playsound==1.2.2")
        else:
            logger.info("Audio system ready (playsound)")
        
        # Initialize Pipeline Stages
        self.collector = DataCollector(TARGET_NAME)
        self.fusion = SensorFusion()
        self.smoother = DataSmoother()
        self.serializer = Serializer()
        self.transmitter = DataTransmitter(("127.0.0.1", 5005))
        self.feedback_handler = FeedbackHandler()

    def _play_sound(self, filepath: str):
        if not SOUND_AVAILABLE: return
        import time
        now = time.time()
        if now - self._last_sound_time < 2.0: return
        
        with self._sound_lock:
            self._last_sound_time = now
            def play():
                try:
                    logger.info(f"Playing sound: {filepath}")
                    playsound(filepath)
                except Exception as e: 
                    logger.error(f"Sound playback error: {e}")
            threading.Thread(target=play, daemon=True).start()

    # --- BLE Server Callbacks ---
    def _read_request(self, characteristic, **kwargs):
        with self._data_lock:
            return bytearray(self._latest_data)

    def _write_request(self, characteristic, value, **kwargs):
        if characteristic.uuid.lower() == LOCAL_FEEDBACK_UUID.lower():
            logger.info(f"Write to Feedback Char: {value.hex()}")
            asyncio.create_task(self.feedback_handler.handle_command(value))
        else:
            characteristic.value = value

    async def _start_server(self):
        logger.info("Starting Local GATT Server...")
        self._server = BlessServer(name=LOCAL_NAME)
        self._server.read_request_func = self._read_request
        self._server.write_request_func = self._write_request
        await self._server.add_new_service(LOCAL_SERVICE_UUID)
        await self._server.add_new_characteristic(
            LOCAL_SERVICE_UUID, LOCAL_CHAR_UUID,
            GATTCharacteristicProperties.read | GATTCharacteristicProperties.notify,
            self._latest_data, GATTAttributePermissions.readable
        )

        await self._server.add_new_characteristic(
            LOCAL_SERVICE_UUID, LOCAL_FEEDBACK_UUID,
            GATTCharacteristicProperties.write, 
            bytearray([0x00]), GATTAttributePermissions.writeable
        )
        
        await self._server.start()

    # --- The Async Pipeline Processing ---
    async def _on_remote_notification(self, sender: int, raw_data: bytearray):
        """
        CORE PIPELINE: 
        Data arrives -> Fusion -> Smoothing -> Serialization -> Transmission
        """
        # 1. Fusion (Filtering)
        quat = self.fusion.process(raw_data)
        
        if quat:
            # 2. Smoothing (Average)
            smoothed_quat = self.smoother.smooth(quat) or quat
            
            # 3. Serialization (Package)
            packet = self.serializer.serialize(smoothed_quat)
            
            # 4. Transmission (UDP & BLE)
            self.transmitter.transmit_udp(smoothed_quat)
            
            # Async Feedback (BLE Notify)
            asyncio.create_task(self.transmitter.transmit_ble_notify(
                self._server, LOCAL_SERVICE_UUID, LOCAL_CHAR_UUID, packet
            ))

            # Update local cache for READ requests
            with self._data_lock:
                self._latest_data = packet
        else:
            logger.warning("Pipeline failed at Fusion stage")

    async def run(self):
        try:
            await self._start_server()
            # Run collector and keepalive
            await asyncio.gather(
                self.collector.collect(self._on_remote_notification),
                self._keepalive()
            )
        except asyncio.CancelledError:
            logger.info("Получен сигнал остановки...")
        finally:
            self._running = False
            if self._server: 
                await self._server.stop()
            logger.info("Выключение завершено.")

    async def _keepalive(self):
        while self._running: await asyncio.sleep(3600)

def main():
    gateway = BLEGateway()
    try:
        asyncio.run(gateway.run())
    except KeyboardInterrupt:
        logger.info("Остановлено пользователем.")

if __name__ == "__main__":
    main()
#!/usr/bin/env python3
"""
BLE Gateway: Принимает данные с ESP32 (пульт) и транслирует их манипулятору.
Требует: pip install bleak bless
На Linux: sudo setcap cap_net_raw+ep /usr/bin/python3 или запуск от root
"""

import asyncio
import logging
import signal
import sys
import threading
from typing import Any, Optional, Union, Dict
from dataclasses import dataclass
from datetime import datetime

from bleak import BleakClient, BleakScanner, BLEDevice
from bleak.exc import BleakError

from bless import (
    BlessServer,
    BlessGATTCharacteristic,
    GATTCharacteristicProperties,
    GATTAttributePermissions,
)

# ==================== Configuration ====================
@dataclass
class Config:
    # ESP32 (пульт) - входящие данные
    ESP32_NAME: str = "ESP32-MPU6050-BLE"
    ESP32_SERVICE_UUID: str = "acc0a4a9-f284-4eac-8fa5-d825c55ce64c"
    ESP32_CHAR_UUID: str = "fc18c54c-2f23-4c05-84bd-338ca880b786"
    
    # Манипулятор - исходящие данные
    GATEWAY_NAME: str = "BLE-Gateway"
    OUTPUT_SERVICE_UUID: str = "6938e8b6-77d8-44e4-ab9d-d27918908cb8"  # Можно тот же сервис
    OUTPUT_CHAR_UUID: str = "e869108c-f2db-4772-a6ba-380a0761ef24"    # Другая характеристика
    
    # Таймауты
    SCAN_TIMEOUT: float = 10.0
    RECONNECT_DELAY: float = 5.0
    NOTIFICATION_TIMEOUT: float = 30.0

# ==================== Logging ====================
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(name)s: %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('ble_gateway.log')
    ]
)
logger = logging.getLogger(__name__)

# ==================== Data Processor ====================
class DataProcessor:
    """
    Обработчик данных между ESP32 и манипулятором.
    Здесь реализуется бизнес-логика преобразования данных.
    """
    
    @staticmethod
    def process(raw_data: bytearray) -> bytearray:
        """
        Обработка сырых данных от ESP32.
        
        Args:
            raw_data: Данные от пульта
            
        Returns:
            Обработанные данные для манипулятора
        """
        try:
            # Пример: парсинг как строки
            # text = raw_data.decode('utf-8')
            
            # Пример: добавление timestamp
            timestamp = datetime.now().isoformat().encode()
            processed = bytearray(raw_data) + b'|' + timestamp
            
            logger.debug(f"Processed {len(raw_data)} bytes -> {len(processed)} bytes")
            return processed
            
        except Exception as e:
            logger.error(f"Processing error: {e}")
            # В случае ошибки возвращаем исходные данные
            return raw_data

# ==================== BLE Gateway ====================
class BLEGateway:
    def __init__(self, config: Config):
        self.config = config
        self.running = False
        self.bless_server: Optional[BlessServer] = None
        self.bleak_client: Optional[BleakClient] = None
        self.processor = DataProcessor()
        
        # Текущее значение для манипулятора
        self._current_value: bytearray = bytearray(b'INIT')
        self._value_lock = asyncio.Lock()
        
        # Очередь для данных от ESP32
        self._data_queue: asyncio.Queue[bytearray] = asyncio.Queue(maxsize=100)
        
        # События управления
        self._shutdown_event = asyncio.Event()
        self._server_started = asyncio.Event()
        
        # Платформенно-зависимый trigger для bless
        if sys.platform in ["darwin", "win32"]:
            self._bless_trigger: Union[asyncio.Event, threading.Event] = threading.Event()
        else:
            self._bless_trigger = asyncio.Event()

    # ---------- Server (Manipulator) ----------
    def _read_request(self, characteristic: BlessGATTCharacteristic, **kwargs) -> bytearray:
        """Callback на чтение характеристики манипулятором"""
        logger.debug(f"Read request from manipulator: {self._current_value.hex()}")
        return self._current_value

    def _write_request(self, characteristic: BlessGATTCharacteristic, value: Any, **kwargs):
        """Callback на запись от манипулятора (если нужен двусторонний обмен)"""
        logger.info(f"Write from manipulator: {value}")
        # Здесь можно обработать команды от манипулятора к ESP32

    async def _start_server(self):
        """Запуск BLE Peripheral для подключения манипулятора"""
        logger.info(f"Starting BLE Server: {self.config.GATEWAY_NAME}")
        
        try:
            server = BlessServer(
                name=self.config.GATEWAY_NAME,
                loop=asyncio.get_running_loop()
            )
            server.read_request_func = self._read_request
            server.write_request_func = self._write_request
            
            self.bless_server = server
            
            # Добавляем сервис
            await server.add_new_service(self.config.OUTPUT_SERVICE_UUID)
            
            # Характеристика: Read + Notify для манипулятора
            char_flags = (
                GATTCharacteristicProperties.read |
                GATTCharacteristicProperties.notify
            )
            permissions = GATTAttributePermissions.readable
            
            await server.add_new_characteristic(
                self.config.OUTPUT_SERVICE_UUID,
                self.config.OUTPUT_CHAR_UUID,
                char_flags,
                self._current_value,
                permissions
            )
            
            await server.start()
            logger.info(f"Server advertising. Waiting for manipulator...")
            logger.info(f"Service UUID: {self.config.OUTPUT_SERVICE_UUID}")
            logger.info(f"Char UUID: {self.config.OUTPUT_CHAR_UUID}")
            
            self._server_started.set()
            
            # Держим сервер запущенным
            await self._shutdown_event.wait()
            
        except Exception as e:
            logger.error(f"Server error: {e}", exc_info=True)
            raise
        finally:
            if self.bless_server:
                await self.bless_server.stop()

    async def _update_output_value(self, data: bytearray):
        """Обновление значения для манипулятора с уведомлением"""
        async with self._value_lock:
            self._current_value = data
            
            if self.bless_server:
                try:
                    # Обновляем значение и отправляем notification подписчикам
                    self.bless_server.update_value(
                        self.config.OUTPUT_SERVICE_UUID,
                        self.config.OUTPUT_CHAR_UUID
                    )
                    logger.debug(f"Notified manipulator: {data.hex()[:20]}...")
                except Exception as e:
                    logger.error(f"Failed to update characteristic: {e}")

    # ---------- Client (ESP32) ----------
    async def _find_esp32(self) -> Optional[BLEDevice]:
        """Сканирование с частичным совпадением имени"""
        logger.debug(f"🔍 Scanning for '{self.config.ESP32_NAME}'...")
        
        try:
            # ⚠️ ВАЖНО: return_adv=True для доступа к advertising data
            discovered = await BleakScanner.discover(
                timeout=self.config.SCAN_TIMEOUT,
                return_adv=True
            )
            
            self._last_found_devices = []
            target_device = None
            
            logger.info(f"📡 Found {len(discovered)} device(s):")
            
            for address, (device, adv_data) in discovered.items():
                name = device.name or "Unknown"
                rssi = adv_data.rssi if adv_data else "N/A"
                
                self._last_found_devices.append({
                    'address': address,
                    'name': name,
                    'rssi': rssi
                })
                
                logger.info(f"   {address} - {name} ({rssi} dBm)")
                
                # ⚠️ Частичное совпадение (case-insensitive)
                if self.config.ESP32_NAME.lower() in name.lower():
                    logger.info(f"   ⭐ MATCHES TARGET: {name}")
                    target_device = device
                    self._found_address = address
                    break
            
            if target_device:
                logger.info(f"✅ Found ESP32 at address: {target_device.address}")
            else:
                logger.warning(f"⚠️ Device '{self.config.ESP32_NAME}' not found in {len(discovered)} devices")
                
            return target_device
            
        except Exception as e:
            logger.error(f"❌ Scan failed: {e}")
            return None

    def _notification_handler(self, sender: str, data: bytearray):
        """Обработчик уведомлений от ESP32 (вызывается в отдельном треде)"""
        # Логируем полученные данные сразу
        logger.info(f"📥 Received from ESP32: {data.hex()} (length: {len(data)} bytes)")
        
        # Если хотим видеть в виде строки (если данные текстовые):
        try:
            logger.info(f"📥 Received text: {data.decode('utf-8')}")
        except:
            logger.info(f"📥 Received hex: {data.hex()}")
        
        # Передаем в основной цикл событий
        asyncio.create_task(self._handle_incoming_data(data))

    async def _handle_incoming_data(self, data: bytearray):
        """Помещение данных в очередь обработки"""
        try:
            # Неблокирующее добавление с отбрасыванием старых данных при переполнении
            if self._data_queue.full():
                old = self._data_queue.get_nowait()
                logger.warning("Queue full, dropped old data")
            
            await self._data_queue.put(data)
            logger.debug(f"Queued data from ESP32: {data.hex()[:30]}...")
            
        except Exception as e:
            logger.error(f"Queue error: {e}")

    async def _esp32_client_loop(self):
        """Основной цикл подключения к ESP32 с автопереподключением"""
        retry_delay = self.config.RECONNECT_DELAY
        
        while self.running:
            try:
                device = await self._find_esp32()
                
                if not device:
                    logger.info(f"Retrying in {retry_delay}s...")
                    await asyncio.sleep(retry_delay)
                    continue
                
                await self._connect_and_listen(device)
                # Если дошли сюда, значит disconnect, сбрасываем задержку
                retry_delay = self.config.RECONNECT_DELAY
                
            except Exception as e:
                logger.error(f"Client loop error: {e}")
                await asyncio.sleep(retry_delay)
                # Exponential backoff
                retry_delay = min(retry_delay * 2, 60)

    async def _connect_and_listen(self, device: BLEDevice):
        """Подключение и прослушивание характеристики ESP32"""
        async with BleakClient(device, timeout=20.0) as client:
            self.bleak_client = client
            logger.info(f"Connected to ESP32 {device.address}")
            
            if not client.is_connected:
                return
            
            try:
                # Проверяем сервисы
                services = client.services
                target_char = None
                
                for service in services:
                    if service.uuid.lower() == self.config.ESP32_SERVICE_UUID.lower():
                        for char in service.characteristics:
                            if char.uuid.lower() == self.config.ESP32_CHAR_UUID.lower():
                                target_char = char
                                break
                
                if not target_char:
                    logger.error("Target characteristic not found on ESP32")
                    return
                
                # Подписываемся на уведомления если поддерживаются
                if "notify" in target_char.properties:
                    await client.start_notify(
                        self.config.ESP32_CHAR_UUID,
                        self._notification_handler
                    )
                    logger.info("Subscribed to ESP32 notifications")
                    
                    # Ждем пока соединение активно
                    while client.is_connected and self.running:
                        await asyncio.sleep(1)
                        
                    await client.stop_notify(self.config.ESP32_CHAR_UUID)
                    
                else:
                    # Polling mode
                    logger.info("Using polling mode")
                    while client.is_connected and self.running:
                        data = await client.read_gatt_char(self.config.ESP32_CHAR_UUID)
                        await self._handle_incoming_data(data)
                        await asyncio.sleep(0.1)
                        
            except BleakError as e:
                logger.error(f"BLE communication error: {e}")
            finally:
                logger.info("Disconnected from ESP32")

    # ---------- Processing ----------
    async def _processing_loop(self):
        """Цикл обработки данных из очереди"""
        logger.info("Processing loop started")
        
        while self.running:
            try:
                # Ждем данные с таймаутом для проверки флага running
                data = await asyncio.wait_for(
                    self._data_queue.get(),
                    timeout=1.0
                )
                
                # Обработка
                processed = self.processor.process(data)
                
                # Обновление сервера (после того как он стартанул)
                await self._server_started.wait()
                await self._update_output_value(processed)
                
            except asyncio.TimeoutError:
                continue
            except Exception as e:
                logger.error(f"Processing error: {e}", exc_info=True)

    # ---------- Lifecycle ----------
    async def start(self):
        """Запуск всех компонентов"""
        logger.info("=== BLE Gateway Starting ===")
        self.running = True
        
        # Регистрация обработчиков сигналов
        self._loop = asyncio.get_running_loop()
        loop = asyncio.get_running_loop()
        for sig in (signal.SIGINT, signal.SIGTERM):
            loop.add_signal_handler(sig, lambda: asyncio.create_task(self.shutdown()))
        
        try:
            # Запускаем сервер, клиент и обработчик параллельно
            await asyncio.gather(
                self._start_server(),
                self._esp32_client_loop(),
                self._processing_loop(),
                return_exceptions=True
            )
        except asyncio.CancelledError:
            logger.info("Main tasks cancelled")
        finally:
            await self.shutdown()

    async def shutdown(self):
        """Graceful shutdown"""
        if not self.running:
            return
            
        logger.info("Initiating shutdown...")
        self.running = False
        self._shutdown_event.set()
        
        # Закрываем клиентское соединение
        if self.bleak_client and self.bleak_client.is_connected:
            try:
                await self.bleak_client.disconnect()
                logger.info("BLE client disconnected")
            except Exception as e:
                logger.error(f"Error disconnecting client: {e}")
        
        # Останавливаем сервер
        if self.bless_server:
            try:
                await self.bless_server.stop()
                logger.info("BLE server stopped")
            except Exception as e:
                logger.error(f"Error stopping server: {e}")
        
        # Очищаем очередь
        while not self._data_queue.empty():
            try:
                self._data_queue.get_nowait()
            except asyncio.QueueEmpty:
                break
        
        logger.info("Shutdown complete")

# ==================== Main ====================
def main():
    config = Config()
    
    # Можно переопределить через переменные окружения
    if 'ESP32_NAME' in os.environ:
        config.ESP32_NAME = os.environ['ESP32_NAME']
    
    gateway = BLEGateway(config)
    
    try:
        asyncio.run(gateway.start())
    except KeyboardInterrupt:
        logger.info("Interrupted by user")
    except Exception as e:
        logger.critical(f"Fatal error: {e}", exc_info=True)
        sys.exit(1)

if __name__ == "__main__":
    import os
    main()
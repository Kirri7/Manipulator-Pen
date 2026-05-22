import sys
import logging
import asyncio
import threading
from typing import Any, Union

from bless import (
    BlessServer,
    BlessGATTCharacteristic,
    GATTCharacteristicProperties,
    GATTAttributePermissions,
)

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(name=__name__)

# Synchronization logic
trigger: Union[asyncio.Event, threading.Event]
if sys.platform in ["darwin", "win32"]:
    trigger = threading.Event()
else:
    # We will initialize this inside the async run function to ensure it's tied to the loop
    trigger = None 

def read_request(characteristic: BlessGATTCharacteristic, **kwargs) -> bytearray:
    logger.debug(f"Reading {characteristic.uuid}")
    return characteristic.value

def write_request(characteristic: BlessGATTCharacteristic, value: Any, **kwargs):
    characteristic.value = value
    logger.debug(f"Char value set to {characteristic.value}")
    if characteristic.value == b"\x0f":
        logger.debug("NICE")
        # Use the global trigger
        global trigger
        if isinstance(trigger, threading.Event):
            trigger.set()
        elif isinstance(trigger, asyncio.Event):
            # Since write_request is a callback, we must use call_soon_threadsafe 
            # if it's called from a different thread, but Bless usually runs in the loop.
            trigger.set()

async def run():
    global trigger
    
    # Initialize the asyncio trigger correctly within the running loop
    if sys.platform not in ["darwin", "win32"]:
        trigger = asyncio.Event()
    else:
        trigger = threading.Event()

    trigger.clear()
    
    my_service_name = "Test Service"
    # In modern bless, passing the loop explicitly is often unnecessary 
    # as it picks up the running loop
    server = BlessServer(name=my_service_name)
    server.read_request_func = read_request
    server.write_request_func = write_request

    # Add Service
    my_service_uuid = "6938e8b6-77d8-44e4-ab9d-d27918908cb8"
    await server.add_new_service(my_service_uuid)

    # Add a Characteristic
    my_char_uuid = "e869108c-f2db-4772-a6ba-380a0761ef24"
    char_flags = (
        GATTCharacteristicProperties.read
        | GATTCharacteristicProperties.write
        | GATTCharacteristicProperties.indicate
    )
    permissions = GATTAttributePermissions.readable | GATTAttributePermissions.writeable
    await server.add_new_characteristic(
        my_service_uuid, my_char_uuid, char_flags, None, permissions
    )

    await server.start()
    logger.info("Сервер запущен и готов к работе вечно...")

    try:
        # Просто держим цикл запущенным, пока не прервут программу
        while True:
            await asyncio.sleep(3600) 
    except asyncio.CancelledError:
        await server.stop()

if __name__ == "__main__":
    try:
        asyncio.run(run())
    except KeyboardInterrupt:
        logger.info("Server stopped by user")
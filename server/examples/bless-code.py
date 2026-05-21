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

# Synchronization method based on platform
if sys.platform in ["darwin", "win32"]:
    trigger = threading.Event()
else:
    # Note: In a purely async environment, we'll use an asyncio.Event 
    # inside the run function instead of this global check for better stability
    trigger = None 

def read_request(characteristic: BlessGATTCharacteristic, **kwargs) -> bytearray:
    logger.debug(f"Reading {characteristic.uuid}")
    return characteristic.value

def write_request(characteristic: BlessGATTCharacteristic, value: Any, **kwargs):
    characteristic.value = value
    logger.debug(f"Char value set to {characteristic.value}")
    if characteristic.value == b"\x0f":
        logger.debug("NICE")
        # Check if we are using threading or asyncio trigger
        if isinstance(trigger, threading.Event):
            trigger.set()
        elif trigger is not None:
            trigger.set()

async def run():
    # Create the event for this specific loop
    async_trigger = asyncio.Event()
    
    # Global trigger fallback for the write_request function
    global trigger
    trigger = async_trigger

    # Instantiate the server
    my_service_name = "Test Service"
    # In modern bless, passing the loop explicitly is often unnecessary 
    # as it picks up the running loop
    server = BlessServer(name=my_service_name)
    server.read_request_func = read_request
    server.write_request_func = write_request

    # Add Service
    my_service_uuid = "acc0a4a9-f284-4eac-8fa5-d825c55ce64c"
    await server.add_new_service(my_service_uuid)

    # Add a Characteristic
    my_char_uuid = "fc18c54c-2f23-4c05-84bd-338ca880b786"
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
    logger.info(f"Advertising. Write '0x0F' to: {my_char_uuid}")
    
    # Wait for the write request
    await async_trigger.wait()

    await asyncio.sleep(2)
    logger.debug("Updating characteristic value...")
    
    # CORRECTED: update_value takes (characteristic_uuid, value)
    # Your previous code passed the service_uuid by mistake
    server.update_value(my_char_uuid, b"\x01") 
    
    await asyncio.sleep(5)
    await server.stop()

if __name__ == "__main__":
    try:
        asyncio.run(run())
    except KeyboardInterrupt:
        logger.info("Server stopped by user")
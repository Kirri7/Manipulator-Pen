#!/usr/bin/env python3
"""
BLE Client with device scanning - Fixed connection handling
Requires: pip install bleak
"""

import asyncio
import json
from bleak import BleakClient, BleakScanner

# UUIDs from your Arduino BLE device
SERVICE_UUID = "acc0a4a9-f284-4eac-8fa5-d825c55ce64c"
CHARACTERISTIC_UUID = "fc18c54c-2f23-4c05-84bd-338ca880b786"
TARGET_NAME = "ESP32-Sensor-Device"


async def scan_for_devices(timeout: int = 10):
    """Scan for BLE devices and return list"""
    print(f"🔍 Scanning for BLE devices ({timeout} seconds)...")
    print("-" * 60)
    
    discovered_devices = await BleakScanner.discover(
        timeout=timeout,
        return_adv=True
    )
    
    print(f"\nFound {len(discovered_devices)} device(s):\n")
    
    esp32_devices = []
    all_devices = []
    
    for address, (device, adv_data) in discovered_devices.items():
        name = device.name or "Unknown"
        rssi = adv_data.rssi if adv_data else "N/A"
        
        device_info = {
            'address': address,
            'name': name,
            'rssi': rssi,
            'device': device
        }
        
        all_devices.append(device_info)
        
        if TARGET_NAME.lower() in name.lower():
            esp32_devices.append(device_info)
            print(f"  ✅ {address}")
            print(f"     Name: {name}")
            print(f"     RSSI: {rssi} dBm")
            print(f"     ⭐ MATCHES TARGET!")
            print()
        else:
            print(f"  • {address} - {name} ({rssi} dBm)")
    
    print("-" * 60)
    
    return esp32_devices, all_devices


async def main():
    print("=" * 60)
    print("       ESP32 BLE Sensor Client")
    print("=" * 60)
    
    # Step 1: Scan for devices
    esp32_devices, all_devices = await scan_for_devices(timeout=10)
    
    if not esp32_devices:
        print("\n⚠️  Target device not found!")
        print("\nAvailable devices:")
        for i, dev in enumerate(all_devices, 1):
            print(f"  {i}. {dev['address']} - {dev['name']}")
        
        if not all_devices:
            print("  (No devices found)")
            return
        
        choice = input("\nSelect device number (or 0 to cancel): ").strip()
        if choice.isdigit() and 0 < int(choice) <= len(all_devices):
            selected = all_devices[int(choice) - 1]
            esp32_devices = [selected]
        else:
            return
    
    if esp32_devices:
        device = esp32_devices[0]
        address = device['address']
        print(f"\n🎯 Selected: {device['name']} ({address})")
        
        client = None
        try:
            # Step 2: Connect to the device (without context manager)
            print(f"🔌 Connecting to {address}...")
            client = BleakClient(address)
            await client.connect(timeout=10.0)
            
            if client.is_connected:
                print(f"✅ Connected successfully!")
                print(f"   Address: {client.address}")
            else:
                print("❌ Connection failed")
                return
            
            # Step 3: Listen for data
            print(f"\n👂 Starting to listen for sensor data...")
            print("   Press Ctrl+C to stop\n")
            print("=" * 60)
            
            def notification_handler(sender, data: bytes):
                try:
                    data_str = data.decode('utf-8')
                    sensor_data = json.loads(data_str)
                    
                    print(f"\n📡 [{sensor_data.get('counter', 'N/A')}] "
                          f"🌡️ {sensor_data.get('temp', 'N/A')}°C | "
                          f"💧 {sensor_data.get('humidity', 'N/A')}% | "
                          f"🔋 {sensor_data.get('battery', 'N/A')}%")
                          
                except json.JSONDecodeError:
                    print(f"\n📡 Raw: {data.decode('utf-8')}")
                except Exception as e:
                    print(f"\n⚠️  Error: {e}")
            
            # Start notifications
            await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
            print("✅ Notifications enabled - waiting for data...\n")
            
            # Keep listening
            while True:
                await asyncio.sleep(1)
                
        except Exception as e:
            print(f"\n❌ Error: {e}")
        except KeyboardInterrupt:
            print("\n\n⌨️  Stopped by user")
        finally:
            # Step 4: Disconnect properly
            if client and client.is_connected:
                print("\n👋 Disconnecting...")
                await client.disconnect()
                print("✅ Disconnected")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n\n👋 Goodbye!")

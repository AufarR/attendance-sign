# IMPORTANT: Add public.pem file to the same directory as this script (or any other path you specify. Make sure shell workdir contains public.pem file)

import asyncio
from bleak import BleakClient, BleakScanner
import base64
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.serialization import load_pem_public_key
from cryptography.exceptions import InvalidSignature

DEVICE_NAME = "ESP32-Signer"
WRITE_UUID = "abcd1234-0001-0001-0001-abcdef123456"
NOTIFY_UUID = "abcd1234-0002-0002-0002-abcdef123456"
MESSAGE = b"Hello!"  # Must match what ESP32 hashes + signs
PUBLIC_KEY_PATH = "public.pem"  # Your exported public key

signature_received = None

def handle_notification(sender, data):
    global signature_received
    print(f"[BLE] Signature received (base64): {data.decode()}")
    signature_received = base64.b64decode(data)

async def main():
    global signature_received

    print("[BLE] Scanning for ESP32...")
    devices = await BleakScanner.discover()
    esp32 = None
    for d in devices:
        if d.name == DEVICE_NAME:
            esp32 = d
            break

    if not esp32:
        print("ESP32 not found.")
        return

    print(f"[BLE] Connecting to {esp32.address}...")
    async with BleakClient(esp32.address) as client:
        await client.start_notify(NOTIFY_UUID, handle_notification)
        await asyncio.sleep(1)

        print(f"[BLE] Sending message: {MESSAGE.decode()}")
        await client.write_gatt_char(WRITE_UUID, MESSAGE, response=True)

        # Wait for signature
        timeout = 5
        while signature_received is None and timeout > 0:
            await asyncio.sleep(1)
            timeout -= 1

        if signature_received:
            print(f"[BLE] Verifying signature...")
            with open(PUBLIC_KEY_PATH, "rb") as key_file:
                public_key = load_pem_public_key(key_file.read())
            
            try:
                public_key.verify(
                    signature_received,
                    MESSAGE,
                    padding.PKCS1v15(),
                    hashes.SHA256()
                )
                print("✅ Signature is valid.")
            except InvalidSignature:
                print("❌ Signature is NOT valid.")
        else:
            print("❌ No signature received.")

        await client.stop_notify(NOTIFY_UUID)

if __name__ == "__main__":
    asyncio.run(main())

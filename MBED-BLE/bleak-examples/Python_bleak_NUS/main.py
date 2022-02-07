"""
UART Service
-------------
An example showing how to write a simple program using the Nordic Semiconductor
(nRF) UART service.

Source : https://github.com/hbldh/bleak/blob/master/examples/uart_service.py

linux/ubuntu :
enable debug:  export BLEAK_LOGGING=1
disable debug: unset BLEAK_LOGGING
Clear cache :  bluetoothctl remove C0:07:5E:90:E0:08 (the last entry should be the MAC of the server/peripheral)

"""

__Coauthor__  = "Paul van Haastrecht"
__email__     = "paulvha@hotmail.com"
__version__   = "January 2022"

import asyncio
import sys

from bleak import BleakScanner, BleakClient
from bleak.backends.scanner import AdvertisementData
from bleak.backends.device import BLEDevice

UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
UART_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
UART_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

# All BLE devices have MTU of at least 23. Subtracting 3 bytes overhead, we can
# safely send 20 bytes at a time to any device supporting this service.
UART_SAFE_SIZE = 20

async def uart_terminal():
    """This is a simple "terminal" program that uses the Nordic Semiconductor
    (nRF) UART service. It reads from stdin and sends each line of data to the
    remote device. Any data received from the device is printed to stdout.
    """

    def match_nus_uuid(device: BLEDevice, adv: AdvertisementData):
        # This assumes that the device includes the UART service UUID in the
        # advertising data. This test may need to be adjusted depending on the
        # actual advertising data supplied by the device.
        if UART_SERVICE_UUID.lower() in adv.service_uuids:
            return True

        return False

    device = await BleakScanner.find_device_by_filter(match_nus_uuid)

    def handle_disconnect(_: BleakClient):
        """
            handles disconnect situation
        """
        print("Device was disconnected, goodbye.")
        # cancelling all tasks effectively ends the program
        for task in asyncio.all_tasks():
            task.cancel()
        print("\r")
        exit()

    def handle_rx(_: int, data: bytearray):
        """
            handle data received from NUS server
        """
        # print("received:", data)  # debug
        str1 = data.decode()
        print(str1)

    # if MAC was provided on command line use that
    if len(address) > 0:
        dd = address

    # else try to get MAC based on service
    else:
        dd = device

    async with BleakClient(dd, disconnected_callback=handle_disconnect) as client:

        await client.start_notify(UART_TX_CHAR_UUID, handle_rx)

        print("Connected, start typing and press ENTER...")

        loop = asyncio.get_running_loop()

        while True:
            # This waits until you type a line and press ENTER.
            # A real terminal program might put stdin in raw mode so that things
            # like CTRL+C get passed to the remote device.
            data = await loop.run_in_executor(None, sys.stdin.buffer.readline)

            # data will be empty on EOF (e.g. CTRL+D on *nix)
            if not data:
                break

            # some devices, like devices running MicroPython, expect Windows
            # line endings (uncomment line below if needed)
            # data = data.replace(b"\n", b"\r\n")

            await client.write_gatt_char(UART_RX_CHAR_UUID, data)
            # print("sent:", data)

if __name__ == "__main__":

    print("Nordic Semiconductor Uart Service (NUS) client (version {})".format(__version__))

    # if provided take MAC address from command line
    if len(sys.argv) == 2:
        address = sys.argv[1]
        device = address
        print("Looking for peripheral/server with address {}".format(address))
    else:
        print("Looking for peripheral/server with service {}".format(match_nus_uuid))

    try:
        asyncio.run(uart_terminal())
    except asyncio.CancelledError:
        # task is cancelled on disconnect, so we ignore this error
        pass

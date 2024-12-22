"""
This uses the heartrate service and should work concurrently with other BLE services

On the python side, the Bluetooth Low Energy platform Agnostic Klient for Python (Bleak) project
is used for Cross Platform Support and has been tested with windows 10

As a peripheral/server use the sketch : MBED-BLE_example13_gattservHR
"""

__author__  = "Paul van Haastrecht"
__email__   = "paulvha@hotmail.com"
__version__ = "January 2022"

import platform
import logging
import asyncio
from bleak import BleakClient
from bleak import BleakClient
from bleak import _logger as logger
from bleak.uuids import uuid16_dict

HRT_RX_UUID = "00002a37-0000-1000-8000-00805f9b34fb"

def notification_handler(sender, data):
    """
        notification handler which prints the data received.
    """
    print("current heartrate {0}".format(data[1]))

async def run(address, loop):

    async with BleakClient(address, loop=loop) as client:

        # wait for BLE client to be connected
        x = await client.is_connected()
        print("Now Connected: {0}".format(x))

        # wait for data to be sent from client
        await client.start_notify(HRT_RX_UUID, notification_handler)

        while True :

            #give some time to do other tasks
            await asyncio.sleep(0.01)

if __name__ == "__main__":

    # if provided take MAC address from command line
    if len(sys.argv) == 2:
        address = sys.argv[1]
    else:
        # this is MAC of the peripheral/server device hardcoded
        address = ("C0:07:5E:90:E0:08")

    print("Heart rate client (version {})".format(__version__))
    print("Looking for peripheral/server with address {}".format(address))

    loop = asyncio.get_event_loop()
    loop.run_until_complete(run(address, loop))

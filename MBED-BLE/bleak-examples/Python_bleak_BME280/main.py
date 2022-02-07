#!/usr/bin/python3
# -*- coding: UTF-8 -*-

"""
Paul van Haastrecht - January 2022 / version 1.0

Example of cross platform data transmission between Artemis CORDIO BME280 peripheral/server and Python using the Bleak Project

The client is trying to connect to an BLE peripheral/server based on the device address. Once connected it will subscribe
to the notify characteristic to regular received BME280 data, as measured by the Artemis/Apollo3 sketch MBED-BLE_example16_gattservBME280.
It will display a menu of options to the user to request the temperature in Fahrenheit or Celsius, the altitude in Feet or Meters, stop or
re-start sending regular updates and to request an update now. The command is send to the peripheral/server and echo-ed back on Notify
instead of BME280 data.

On the python side, the Bluetooth Low Energy platform Agnostic Klient for Python (Bleak) project
is used for Cross Platform Support and has been tested with Ubuntu 20.04

linux/ubuntu :
enable debug:  export BLEAK_LOGGING=1
disable debug:  unset BLEAK_LOGGING
"""

__author__  = "Paul van Haastrecht"
__email__   = "paulvha@hotmail.com"
__version__ = "January 2022"

import platform
import logging
import asyncio
import struct                       # data parsing
from bleak import BleakClient
from bleak import BleakClient
from bleak import _logger as logger
from bleak.uuids import uuid16_dict
import os                           # for keyboard read

if os.name == 'nt':
    import msvcrt
else:
    import sys, select

"""
Define the service and characteristics UUID
"""

# BME280 service
BME280ServiceUUID = "19B10010-E8F2-537E-4F6C-D104768A1214"

# Command  characteristic
BME280_rw_UUID = "19B10011-E8F2-537E-4F6C-D104768A1214"

# Notify / data characteristic
BME280_Not_UUID = "19B10012-E8F2-537E-4F6C-D104768A1214"

"""
Define the Menu selection to be sent to peripheral / server
"""
REQUEST_METERS = b'\x01'
REQUEST_FEET = b'\x02'
REQUEST_CELSIUS = b'\x03'
REQUEST_FRHEIT= b'\x04'
REQUEST_STOP =b'\x05'
REQUEST_START = b'\x06'
REQUEST_NOW = b'\x07'
REQUEST_DISCONNECT = b'\x08'

#***********************************************************************
# Defined the structure for the data to exchange on the peripheral/server.
# This structure must be the same on the central and peripheral
# max 20 bytes ( MTU size 23 - 3 overhead)
# used unions to stay within the 20 bytes.
#***********************************************************************
"""
struct data_to_exchange {
  // BME280 data
  float humidity;
  float pressure;
  float altitude;
  float temperature,
  union {
    uint8_t meter;       // if true altitude is in meter, else in feet
    uint8_t CmdStat;
  };
  union {
    uint8_t celsius;     // if true temperature is celsius, else fahrenheit
    uint8_t MagicNumber;
  };
};
"""
#indication that received data is response to earlier sent command
MAGIC_CMD = 207  # 0xCF

#***********************************************************************
_instruction = 0

def kbhit():
    ''' Returns True if a keypress is waiting to be read in stdin, False otherwise.
        https://stackoverflow.com/questions/292095/polling-the-keyboard-detect-a-keypress-in-python
        https://github.com/simondlevy/kbhit/blob/main/kbhit.py
    '''
    if os.name == 'nt':
        return msvcrt.kbhit()
    else:
        dr,dw,de = select.select([sys.stdin], [], [], 0)
        return dr != []

def kbgetch():
    """
        Read keyboard input.
        https://github.com/simondlevy/kbhit/blob/main/kbhit.py
    """
    if os.name == 'nt':
        return msvcrt.getch().decode('utf-8')

    else:
        return sys.stdin.read(1)

def display_menu():
    '''
        Display the menu options.
    '''
    print("\n1.  Request altitude in meters")
    print("2.  Request altitude in feet")
    print("3.  Request temperature in Celsius")
    print("4.  Request temperature in Fahrenheit")
    print("5.  Request STOP sending data")
    print("6.  Request (re)START sending data")
    print("7.  Request NOW sending data")
    print("8.  Disconnect and exit client")
    print("    Or just wait\n")

def SelectionInstruction(i):
    """
        Read the Instruction to send based on keyboard input
        https://hackr.io/blog/python-conditional-statements-switch-if-else
    """
    switcher={
        1:REQUEST_METERS,
        2:REQUEST_FEET,
        3:REQUEST_CELSIUS,
        4:REQUEST_FRHEIT,
        5:REQUEST_STOP,
        6:REQUEST_START,
        7:REQUEST_NOW,
        8:REQUEST_DISCONNECT
    }

    # lookup value or return \x00
    return switcher.get(i,b'\x00')

def GetMenuSelection():
    """
        Read the keyboard input for menu selection
    """
    cnt = 0
    inputs = ""
    while cnt < 9:
        c = kbgetch()
        if ord(c) == 13: # skip 0x0d
            continue

        if ord(c) == 10: # if 0x0a, new line
            break
        inputs += c
        cnt += 1

    if len(inputs) > 0:
        #convert to number
        v = int(inputs)
        global _instruction
        _instruction = SelectionInstruction(v)
        if _instruction != b'\x00': return True
        print("invalid input {}".format(inputs))

    display_menu()
    return False

def ShowServerReturn(i):
    """Display feedback from server on command
    """
    if i == 1:
        print("Reporting Altitude in meters")
    elif i == 2:
        print("Reporting Altitude in feet")
    elif i == 3:
        print("Reporting Temperature in Celsius")
    elif i == 4:
        print("Reporting Temperature in Fahrenheit")
    elif i == 5:
        print("Stop sending updates")
    elif i == 6:
        print("re(start)sending updates")
    elif i == 7:
        print("Now sending update")
    elif i == -1:
        print("Invalid selection")
    else:
        print("Unknown response {}".format(i))

def notify_handler(sender, data):
    """notification handler which prints data received.
       It is either BME280 data or feedback on earlier sent command
    """
    #debug only
    #print("packetlen {0}".format(len(data)))

    # unpack binary data in variables
    # Data is received as 20 bytes (size of MTU). only 18 are used, hence dummy byte
    humidity, pressure, altitude, temperature, meter, MagicNumber, dummy = struct.unpack('=ffffBBH', data)

    if MagicNumber != MAGIC_CMD:
        # now print data
        if meter == 1:
            height = "Meter"
        else:
            height = "Feet"

        if MagicNumber == 1:
            temp = "*C"
        else:
            temp = "*F"

        print("Temperature\t{:.2f} {}".format(temperature, temp))
        print("Humidity\t{:.2f}%".format(humidity))
        print("Pressure\t{:.2f} hPa".format(pressure))
        print("altitude\t{:.2f} {}\n".format(altitude, height))

    else:
        #server return on earlier sent command
        ShowServerReturn(meter)
        # re-display
        display_menu()

async def run(address, loop):
    """
        Here is really starts. creating a client and start scanning
        looking to find a device based on the address. It is then connected
        as a client and starting notify.
        As part of the main loop it will check for any local keyboard
        input and if available the relevant command will be send to the
        peripheral/server or perform a disconnect & exit
    """
    async with BleakClient(address, loop=loop) as client:

        #wait for BLE client to be connected
        x = await client.is_connected()
        print("Connected: {0}".format(x))
        display_menu()

        #wait for data to be sent from client
        await client.start_notify(BME280_Not_UUID, notify_handler)

        # main loop
        while True :

            #give some time to do other tasks
            await asyncio.sleep(0.05)

            # check for keyboard
            if kbhit():
                # if valid keyboard input
                if GetMenuSelection():
                    global _instruction
                    if _instruction == REQUEST_DISCONNECT:
                        print("waiting for disconnect");
                        await client.disconnect()
                        exit()

                    await client.write_gatt_char(BME280_rw_UUID,_instruction)

if __name__ == "__main__":

    # if provided take MAC address from command line
    if len(sys.argv) == 2:
        address = sys.argv[1]
    else:
        # this is MAC of the peripheral/server device hardcoded
        address = ("C0:07:5E:90:E0:08")

    print("BME280 client (version {})".format(__version__))
    print("Looking for peripheral/server with address {}".format(address))
    loop = asyncio.get_event_loop()
    loop.run_until_complete(run(address, loop))

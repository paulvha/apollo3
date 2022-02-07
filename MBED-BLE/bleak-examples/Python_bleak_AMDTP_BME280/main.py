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
disable debug: unset BLEAK_LOGGING
Clear cache :  bluetoothctl remove C0:07:5E:90:E0:08 (the last entry should be the MAC of the server/peripheral)
"""

__author__  = "Paul van Haastrecht"
__email__   = "paulvha@hotmail.com"
__version__ = "January 2022"

import platform
import logging
import asyncio
import struct                       # data parsing
from bleak import BleakClient
from amdtpc import AmdtpClient
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

# AMDTP service
ATT_UUID_AMDTP_SERVICE = "00002760-08C2-11E1-9073-0E8AC72E1011"

# Command /ack SENDING characteristic
ATT_UUID_AMDTP_RX = "00002760-08C2-11E1-9073-0E8AC72E0011"

# Notify / data characteristic
ATT_UUID_AMDTP_TX = "00002760-08C2-11E1-9073-0E8AC72E0012"

# /ack receiving characteristic
ATT_UUID_AMDTP_ACK = "00002760-08C2-11E1-9073-0E8AC72E0013"

"""
Define the Menu selection to be sent to peripheral / server
"""
REQUEST_METERS =  0x1
REQUEST_FEET =    0x2
REQUEST_CELSIUS = 0x3
REQUEST_FRHEIT=   0x4
REQUEST_STOP =    0x5
REQUEST_START =   0x6
REQUEST_NOW =     0x7
REQUEST_DISCONNECT = 0x8

#***********************************************************************
# Defined the structure for the data to exchange on the peripheral/server.
# This structure must be the same on the central and peripheral
#***********************************************************************
"""
struct data_to_exchange {
  // BME280 data
  float humidity;
  float pressure;
  float altitude;
  float temperature;
  uint8_t meter;          // if true altitude is in meter, else in feet
  uint8_t celsius;        // if true temperature is celsius, else fahrenheit

  // command feedback
  int8_t CmdStat;          // echo of earlier send commmand or -1 in case of error
  uint8_t MagicNumber;     // magic number
};
"""
#indication that received data is response to earlier sent command
MAGIC_CMD = 207  # 0xCF
#***********************************************************************
_instruction = 0            # keyboard read result
CentralBuf = []             # store data to send to central
CentralBufReady = 0         # indicate new data has available to send
ShowData = 0                # Show sending / receiving data

def kbhit():
    """
        Returns True if a keypress is waiting to be read in stdin, False otherwise.
        https://stackoverflow.com/questions/292095/polling-the-keyboard-detect-a-keypress-in-python
        https://github.com/simondlevy/kbhit/blob/main/kbhit.py
    """
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
    """
        Display the menu options.
    """
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

    # lookup value or return 0x0
    return switcher.get(i, 0x0)

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

        if _instruction == REQUEST_DISCONNECT:
            return True

        elif _instruction != 0x0:
            buf = []
            buf.append(_instruction)
            tpc.AmdtpSendData(buf,1)
            return True

        print("invalid input {}".format(inputs))

    display_menu()
    return False

def ShowServerReturn(i):
    """
        Display feedback from peripheral / server on command
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

#**********************************************************************
# AMDTP HANDLING
#**********************************************************************
def notify_handler(sender, data):
    """notification handler which prints data received.
       It is either BME280 data or feedback on earlier sent command
    """

    if ShowData == 1:
        print("Received new data")
        i = 0

        while (i < len(data)):
            print(hex(data[i]))
            i += 1

    tpc.AmdtpReceivePkt(data, len(data))

def receive_data(data, len):
    """
        called from amdtpc.py when complete data package has been received
        from the peripheral / server
    """
    # unpack binary data in variables
    values = bytearray(data)
    humidity, pressure, altitude, temperature, meter, celsius, cmdstat, MagicNumber = struct.unpack('=ffffBBBB', values)

    if MagicNumber != MAGIC_CMD:
        # now print data
        if meter == 1:
            height = "Meter"
        else:
            height = "Feet"

        if celsius == 1:
            temp = "*C"
        else:
            temp = "*F"

        print("Temperature\t{:.2f} {}".format(temperature, temp))
        print("Humidity\t{:.2f} %".format(humidity))
        print("Pressure\t{:.2f} hPa".format(pressure))
        print("altitude\t{:.2f} {}\n".format(altitude, height))

    else:
        # server return on earlier sent command
        ShowServerReturn(cmdstat)
        # re-display
        display_menu()

def send_central(data, len):
    """
        called from amdtpc.py to have formatted data
        send to the peripheral / server
    """
    if ShowData == 1:
        print ("in send_central len {0}".format(len))

    global CentralBuf
    CentralBuf = []     # reset
    i = 0

    while (i < len):
        CentralBuf.append(data[i])  # add to buffer to be send

        if ShowData == 1:
            print(hex(data[i]))

        i += 1

    global CentralBufReady
    CentralBufReady = 1

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

        # wait for BLE client to be connected
        x = await client.is_connected()
        if ShowData == 1:
            print("Connected: {0}".format(x))
        display_menu()

        # wait for data to be sent from client
        await client.start_notify(ATT_UUID_AMDTP_TX, notify_handler)

        # wait for receiving ack knowledgements enabled
        await client.start_notify(ATT_UUID_AMDTP_ACK, notify_handler)

        # main loop
        while True :

            # give some time to do other tasks
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

            # anything to send to central
            global CentralBufReady
            if CentralBufReady == 1:
                CentralBufReady = 0
                global CentralBuf
                await client.write_gatt_char(ATT_UUID_AMDTP_RX,CentralBuf)

if __name__ == "__main__":

    # Setup class (with call backs)
    tpc = AmdtpClient(Received_data_callback = receive_data, send_central_callback = send_central) # , debug = True)

    # if provided take MAC address from command line
    if len(sys.argv) == 2:
        address = sys.argv[1]
    else:
        # this is MAC of the peripheral/server device hardcoded
        address = ("C0:07:5E:90:E0:08")

    print("AMDTP - BME280 client (version {})".format(__version__))
    print("Looking for peripheral/server with address {}".format(address))
    loop = asyncio.get_event_loop()
    loop.run_until_complete(run(address, loop))

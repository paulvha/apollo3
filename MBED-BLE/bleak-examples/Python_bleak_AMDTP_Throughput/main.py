#!/usr/bin/python3
# -*- coding: UTF-8 -*-

"""
Paul van Haastrecht - February 2022 / version 1.0

Example of cross platform data transmission between Artemis CORDIO peripheral/server and Python using the Bleak Project

The client is trying to connect to an BLE peripheral/server based on the device address. Once connected it will subscribe
to the notify characteristic. It will then wait for the test data to be send by the peripheral / server. Upon receipt it will,
after a short delay, echo the data back.

To be used with peripheral / server : MBED-BLE_example17_gattserv_AMDTP_troughput

Can be started as python3 main.py "C0:07:5E:90:E0:08" OR python3 main.py. In the last situation the hardcode
targer device address is used (C0:07:5E:90:E0:08)

By entering an 8 + <enter> you can exit the client.

On the python side, the Bluetooth Low Energy platform Agnostic Klient for Python (Bleak) project
is used for Cross Platform Support and has been tested with Ubuntu 20.04

Command for linux/ubuntu :
enable debug :  export BLEAK_LOGGING=1
disable debug:  unset BLEAK_LOGGING
Clear cache  :  bluetoothctl remove C0:07:5E:90:E0:08 (the last entry should be the MAC of the server/peripheral)
"""

__author__  = "Paul van Haastrecht"
__email__   = "paulvha@hotmail.com"
__version__ = "February 2022"

import platform
import logging
import asyncio
#import struct                       # data parsing
from bleak import BleakClient
from amdtpc import AmdtpClient
from bleak import _logger as logger
from bleak.uuids import uuid16_dict
import os                           # for keyboard read
import time

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

# /ack characteristic (receiving only)
ATT_UUID_AMDTP_ACK = "00002760-08C2-11E1-9073-0E8AC72E0013"

#***********************************************************************

# valid keyboard actions
REQUEST_DISCONNECT = 0x8

#***********************************************************************

instruction = 0        # keyboard read result
CentralBuf = []        # store data to send to central
CentralBufReady = 0    # indicate new data has available to send
ReceiveBuf = []        # store received test data
ReceiveBufLen = 0
ReceiveWaitCount = 0   # wait some loops to allow ACK to be send
ShowData = 0           # Show sending (2) or receiving data (1) - DEBUG
SendChunks = False     # is data send on multiple packages?
Milli_seconds = 0       # count

#**********************************************************************
# KEYBOARD HANDLING
#**********************************************************************
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

def GetKeyInput():
    """
        get the keyboard input
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
        global instruction

        #convert to number
        instruction = int(inputs)

        return True

    return False

#**********************************************************************
# AMDTP HANDLING
#**********************************************************************
def notify_handler(sender, data):
    """notification handler which prints data received.
       It is raw test data in AMDTP format
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
        We store this in a buffer to send it later.
    """
    print("Test Data has been received")

    global ReceiveBuf
    global ReceiveBufLen
    global Milli_seconds

    ReceiveBuf = []
    ReceiveBufLen = 0

    while ReceiveBufLen < len:
        ReceiveBuf.append(data[ReceiveBufLen])  # add to buffer to be send

        if ShowData == 2:
            print(hex(data[ReceiveBufLen]))

        ReceiveBufLen += 1

    Milli_seconds = int(round(time.time() * 1000))

def Send_reply():
    """
        echo received data back to peripheral / server
    """

    global SendChunks
    global ReceiveBuf
    global ReceiveBufLen
    global Milli_seconds

    Milli_seconds1 = int(round(time.time() * 1000))
    print("After delay of {0}, echo Data that has been received".format(Milli_seconds1 - Milli_seconds))

    SendChunks = False

    #echo the data back
    ret = tpc.AmdtpSendData(ReceiveBuf, ReceiveBufLen)

    if ret == 1:
        Milli_seconds = int(round(time.time() * 1000))
        print("sending in chunks")
        SendChunks = True
    elif ret == 0:
        print("sending completed")
    else:
        print("Error during sending")

def send_central(data, len):
    """
        called to have formatted data send to the peripheral / server
    """
    if ShowData == 2:
        print ("in send_central len {0}".format(len))

    global CentralBuf
    CentralBuf = []     # reset
    i = 0

    while i < len:
        CentralBuf.append(data[i])  # add to buffer to be send

        if ShowData == 2:
            print(hex(data[i]))

        i += 1

    global CentralBufReady
    CentralBufReady = 1

#**********************************************************************
# MAIN LOOP
#**********************************************************************

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

        # wait for data to be sent from client
        await client.start_notify(ATT_UUID_AMDTP_TX, notify_handler)

        # wait for receiving ack knowledgements enabled
        await client.start_notify(ATT_UUID_AMDTP_ACK, notify_handler)

        print("Waiting for receiving data or press 8 <enter> to disconnect")

        # main loop
        while True :

            # give some time to do other tasks
            await asyncio.sleep(0.05)

            # check for keyboard input pending
            if kbhit():

                # if valid keyboard input
                if GetKeyInput():
                    global instruction
                    if instruction == REQUEST_DISCONNECT:
                        print("waiting for disconnect");
                        await client.disconnect()
                        exit()
                    else:
                        print("Start MBED-BLE_example17_gattsrc_AMDTP_throughput")
                        print("Waiting for receiving data or press 8 <enter> to disconnect")

            # anything to send to central
            global CentralBufReady
            if CentralBufReady == 1:
                CentralBufReady = 0
                global CentralBuf
                await client.write_gatt_char(ATT_UUID_AMDTP_RX,CentralBuf)

                # in case we are sending in chunks check for complete
                global SendChunks
                global Milli_seconds

                if SendChunks == True:
                    if tpc.AmdtpSendComplete() == True:
                        Milli_seconds1 = int(round(time.time() * 1000))
                        print("sending has been completed in {0}".format(Milli_seconds1 - Milli_seconds))
                        print("Waiting for receiving data or press 8 <enter> to disconnect")
                        SendChunks = False

            # Wait with echo to allow for ACK to be send first
            global ReceiveBufLen
            global ReceiveWaitCount
            if ReceiveBufLen > 0:
                ReceiveWaitCount += 1
                if ReceiveWaitCount > 10:
                    Send_reply()
                    ReceiveWaitCount = 0
                    ReceiveBufLen = 0

if __name__ == "__main__":

    # Setup class (with call backs)
    tpc = AmdtpClient(Received_data_callback = receive_data, send_central_callback = send_central) # , debug = True)

    # if provided take MAC address from command line
    if len(sys.argv) == 2:
        address = sys.argv[1]
    else:
        # this is MAC of the peripheral/server device hardcoded
        address = ("C0:07:5E:90:E0:08")

    print("AMDTP - throughput client (version {})".format(__version__))
    print("Looking for peripheral/server with address {}".format(address))
    loop = asyncio.get_event_loop()
    loop.run_until_complete(run(address, loop))

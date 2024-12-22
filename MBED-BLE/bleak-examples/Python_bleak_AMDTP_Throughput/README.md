# Python_Throughput

Example of cross platform data transmission between Artemis CORDIO BLE peripheral/server and Python using the Bleak Project

Unlike the [Adafruit BLE Desktop application](https://github.com/adafruit/adafruit-bluefruit-le-desktop) which is limited on windows to certain hardware BLE adapters, the use of [Bleak](https://github.com/hbldh/bleak) allows improved connectivity by supporting "Windows 10, version 16299 (Fall Creators Update) or greater". This example has been tested on Ubuntu 20.04

## Overview

The client is trying to connect to an BLE peripheral/server based on the device address. Once connected it will subscribe to the Notify characteristic.

This is a example of using AMD Transfer Protocol (AMDTP) service. With AMDTP you can send larger data sizes (by default up to 512 bytes). This data will be broken down by the sender in smaller packages to the size of the MTU. The smaller pacakages will be controlled exchanged between sender and receiver with the use of the AMDTP. On the receiver side these package will be combined into a single data package and the end-to-end CRC will be checked to make sure the data is correct.

In this example we exchange only a 512 data package to the client and the client will echo back the same package.

This uses should work concurrently with other BLE services such as HID allowing cool features for devices such as BLE keyboards to communicate data over an additional channel - possibly to update configs during runtime, etc...

## Bleak
On the python side, the Bluetooth Low Energy platform Agnostic Klient for Python (Bleak) project is used for Cross Platform Support and has been tested with windows 10. This example has been tested on Ubuntu 20.04.
More info on BLEAK :
[bleak-github](https://github.com/hbldh/bleak)
[bleak-documents](https://bleak.readthedocs.io/en/latest/)

## Getting started
Make sure the Bleak is installed on your system (see https://bleak.readthedocs.io/en/latest/)
Copy the connect of this folder into a folder on your system and change into the directory
Obtain the MAC address,  which is displayed by the peripheral/server sketch MBED-BLE_example16_gattservBME280

EITHER

 python3 main.py Pheripheral_MAC_address: e.g. python3 main.py C0:07:5E:90:E0:08

OR

 Open to edit main.py
 At bottom (around line 281) hardcode the address
 save the file
 start as python3 main.py

## Remarks
* It does happen often that connection times out during discovery. Check the MAC, reposition the Artemis board or check with Bleak debug if that continues
* unfortunately the BLEAK backend does not allow filter device name. So you must use the device address
* Parts are based on the Python Nus https://github.com/coyt/PythonNUS

Paul van Haastrecht
February 2022

# Python_BME280

Example of cross platform data transmission between Artemis CORDIO BME280 peripheral/server and Python using the Bleak Project

Unlike the [Adafruit BLE Desktop application](https://github.com/adafruit/adafruit-bluefruit-le-desktop) which is limited on windows to certain hardware BLE adapters, the use of [Bleak](https://github.com/hbldh/bleak) allows improved connectivity by supporting "Windows 10, version 16299 (Fall Creators Update) or greater". This example has been tested on Ubuntu 20.04

## Overview

The client is trying to connect to an BLE peripheral/server based on the device address. Once connected it will subscribe to the Notify characteristic
for regular BME280 data updates, as measured by the Artemis/Apollo3 with the sketch MBED-BLE_example14_gattservBME280.  Next to that it will display a menu of options to the user to request the temperature in Fahrenheit or Celsius, the altitude in Feet or Meters, stop or re-start sending regular updates and to request an update now. The command is send to the peripheral/server and echo-ed back on notify instead of BME280 data.

This uses should work concurrently with other BLE services such as HID allowing cool features for devices such as BLE keyboards to communicate data over an additional channel - possibly to update configs during runtime, etc...

## Bleak
On the python side, the Bluetooth Low Energy platform Agnostic Klient for Python (Bleak) project is used for Cross Platform Support and has been tested with windows 10. This example has been tested on Ubuntu 20.04.
More info on bleak :
https://github.com/hbldh/bleak
https://bleak.readthedocs.io/en/latest/

## Getting started
Make sure the Bleak is installed on your system (see https://bleak.readthedocs.io/en/latest/)
Copy the connect of this folder into a folder on your system and change into the directory
Obtain the MAC address,  which is displayed by the peripheral/server sketch MBED-BLE_example16_gattservBME280

EITHER

 python3 main.py BME280_MAC_address: e.g. python3 main.py C0:07:5E:90:E0:08

OR

 Open to edit main.py
 At bottom (around line 281) hardcode the address
 save the file
 start as python3 main.py

## Remarks
* It does happen often that connection times out during discovery. Check the MAC, reposition the Artemis board or check with Bleak debug if that continues
* unfortunately the bleak backend does not allow filter device name. So you must use the device address
* Parts are based on the Python Nus https://github.com/coyt/PythonNUS

Paul van Haastrecht
January 2022

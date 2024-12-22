# This reads the heartrate from the Artemis MBED-BLE example13 Heartrate

Unlike the [Adafruit BLE Desktop application](https://github.com/adafruit/adafruit-bluefruit-le-desktop) which is limited on windows to certain hardware BLE adapters, the use of [Bleak](https://github.com/hbldh/bleak) allows improved connectivity by supporting "Windows 10, version 16299 (Fall Creators Update) or greater" as well as UBUNTu 20.04

#### Overview

Quick Test program demonstrating data transmission between MBED BLE libraries running on an Artemis / Apollo3 board and Python running on a PC. THis has been tested on Ubuntu, but there should be no reason why it would not work on other platforms that support bleak

The Python program simply awaits a BLE connection from a hardware device and displays the simulate heartrate.

This should work concurrently with other BLE services such as HID allowing cool features for devices such as BLE keyboards to communicate data over an additional channel - possibly to update configs during runtime, etc...

On the Artemis side - run MBED-BLE_example13_gattservHR

On the python side, the Bluetooth Low Energy platform Agnostic Klient for Python (Bleak) project is used for Cross Platform Support and has been tested with windows 10 and Ubuntu 20.04

#### Based on
PythonNUS a cross platform data transmission between Nordic UART Service (NUS) and Python using the Bleak Project and Adafruit nrf52 Libraries

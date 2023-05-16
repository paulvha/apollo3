# Artemis / Apollo3 examples

## ===========================================================

Different examples for the Apollo3 processor.

## Getting Started
In the different folders are different programs with a readme file.
Also check the .odt files which can be read with nearly any word processor.

## Prerequisites
### Linux/Raspberry PI
Some examples have an optional Bluetooth client that requires
Bluez bluetooth stack : http://www.bluez.org/download/

### Arduino
ArduinoBLE and (optional) BME280
Apollo3 Sparkfun library version (at least) 2.2.1

## Software installation
Obtain the zip and install like any other for the Arduino
CHeck the extras directory for client software information

## Program usage
### Program options
Please see the description in the top of the sketch
For the Linux client software use the --help or -h option

## Versioning

### version 3.20 / May 2023
 * update to SoftwareSerial

### version 3.19 / April 2023
 * added IOS (Apollo3 working as peripheral on SPI or I2C)

### version 3.18 / March 2023
 * Updated MicroSD filemanager to support MicroMod Input and display board

### version 3.17 / March 2023
 * added detailed information and sketch about deepsleep V1.x and V2.x

### Version 3.16 / January 2023
 * update ArduinoBLE_special for V1.2.3
 * added Memory Dump sketch
 * updated Qwicc filemanager 1.4 (with support for ESP32)

### version 3.15 / November 2022
 * updated Qwicc filemanager 1.3
 * added printf (solution to floating point printf)

### version 3.14 / November 2022
 * updated BLE for version 1.2.3 (ArduinoBLE_P)
 * added Ubuntu central for BLE  (ArduinoBLE_P)
 * added fast analog in Analog Special

### version 3.13 / April 2022
 * updated MicroSD Filemanager for more MicroMod processors

### version 3.12 / March 2022
 * updated Artemis Filemanager for MicroSD
 * added Qwicc Openlog filemanager

### version 3.11 / March 2022
 * added Artemis microSD Filemanager

### version 3.10 / February 2022
 * added MBED-BLE

### version 3.09 / december 2021
 * added document on deepdive on I2C and QWIIC button

### Version 3.08 / december 2021
 * added OneWire on Uart

### version 3.7 / November 2021
 * added MLX90614 PWM mode

### Version 3.6 / November 2021
 * added MLX90615 and info about SMBUS compliance

### version 3.5 / May 2021
 * added APgpio for extended control of GPIO

### version 3.4 / April 2021
 * added HB01B0 for Version 2.0.6

### version 3.3 / April 2021
 * added BLE for version 1.2.1 (ArduinoBLE_P)

### version 3.2 / April 2021
 * added SoftwareSerial for Library 2.0.6

### version 3.1 / december 2020
 * update to amdtp-client and amdtp-server on Arduino
 * update to amdtc on linux to work with Bluez 5.55 and stability

### version 3.0.1 / November 2020
 * added example Update_Apollo3_bdaddr (READ REMARKS in the TOP of Sketch)

### version 3.0 / November 2020
 * added files crc.c/crc.h

### Version 3.0 / October 2020
   Based on the complete change in version 2.0.1 the following changes have been made:
 * Removed BLE_button, batt_temp, btble (they are not compatible anymore and obsolete)
 * moved amdtpc to amdtp-client and changed to run on top of ArduinoBLE.
 * moved amdtps to amdtp-server and changed to run on top of ArduinoBLE.
 * update to the linux client to work with amdtp-server.

### Version 2.1 / June 2020
 * added BLE button

### Version 2.0 / March 2020
 * updated the BLE-amdtp between Apollo3 board and Linux
 * added BLE-amdtp between 2 Apollo3 boards
 * updated amdtps, amdtpc Apollo3 and amdtpc Linux
 * tested on Apollo3 version 1.0.30

### Version 1.0.1 / January 2020
 * added batt_temp tested on the edge board but should work on any Apollo3
 * batt_temp will read local battery level, temperature and set battery load resistor
 * batt_temp is NOT using the standard analogRead(), but intializes the ADC module it self

### version 1.0 / January 2020
 * Initial version tested with the edge board
 * btble : will read battery level, temperature and set battery load resistor
 * btble : contains an optional client that has been tested on Ubuntu and Raspberry Pi
 * btble : tested on the orginal version from Sparkfun voor Apollo3 (1.0.23)

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgements
Make sure to have the Apollo3 datasheet from https://cdn.sparkfun.com/assets/learn_tutorials/9/0/9/Apollo3_Blue_MCU_Data_Sheet_v0_9_1.pdf

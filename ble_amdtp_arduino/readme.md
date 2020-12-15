# Exchange data over BLE between two Artemis / Apollo3 boards

## ===========================================================

A solution with server software running on an Apollo3 board and client
another Apollo3 board. Tested with Arduino IDE 1.8.13

Another version works with a client/host running on Bluez5.55 linux ![https://github.com/paulvha/apollo3/tree/master/ble_amdtp_raspPi](https://github.com/paulvha/apollo3/tree/master/ble_amdtp_raspPi)

## Getting Started
As part of a project to better understand Bluetooth, I have worked
connecting boards from Sparkfun based on Apollo3

The connection is using the lightweight, propriety AMD Transaction Protocol. (AMDTP)
See the extras folder for more information about this protocol.

For detailed information about the solution, please read the amdtp-arduino.odt in the extras folder.

## Prerequisites

### Server :
If you plan to add an BME280 on the server
<br> BME280   : https://github.com/sparkfun/SparkFun_BME280_Arduino_Library

Make sure to have at least version 2.0.3 or higher for the Apollo3 Arduino software

ArduinoBLE library must be available in the IDE.

### Client
Make sure to have at least version 2.0.3 or higher for the Apollo3 Arduino software

ArduinoBLE library must be available in the IDE.

## Software installation
Please read the amdtp-arduino.odt in the extras folder.

## Program usage
### Server Program options
Please see the description in the top of the sketch and read the documentation (odt)

### client Program options
Please see the description in the top of the sketch and read the documentation (odt)

## Versioning

### version 3.1 / December 2020
  * update with different ACK-handle and timing to improve stability
  * changed from getTempDegF to getTempDegC (new in 2.0.2)
  * CRC32 is now also defined in Mbed. (new in 2.0.3) Included instruction in CRC32.c/ CRC32.h
  * client/host tested on Arduino Nano 33 BLE Sense with server/slave on Apollo3 ATP

### version 3.0 / October 2020
  * Rewrite as the Server and cLient now run on-top of ArduinoBLE

### version 2.0.2 / April 2020
  * fixed issue with look-up friendly name (provided by Randy Lewis)

### version 2.0.1 / April 2020
  * fixed issue with friendly name beyond 5 BLE devices

### version 2.0 / March 2020
  * updated some typos
  * SmpHandlerInit() update
  * changed ADC pin validation to server
  * added major / minor number
  * added requesting server version numbers
  * update to capture friendly name

### version 1.0 / March 2020
 * Initial version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgments

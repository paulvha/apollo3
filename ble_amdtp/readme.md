# Exchange data over BLE between Artemis/Apollo3 and linux

## ===========================================================

A solution with server software running on an Apollo3 board and client
software running on Linux (tested Raspberry PI or Ubuntu 18.04)

## Getting Started
As part of a project to better understand Bluetooth, I have worked
connecting boards from Sparkfun based on Apollo3 over Bluetooth
with Raspberry Pi and Ubuntu.
The connection is using the lightweight, propriety AMD Transaction Protocol.
See the extras folder for more information about this protocol

For detailed information about the solution, please read the amdtp.odt in the extras folder.

## Prerequisites
### Server :
If you plan to add an BME280  on the server
<br> BME280   : Adafruit_BME280 and Adafruit_Sensor

### Client
Bluez Linux Bluetooth stack http://www.bluez.org/

## Software installation
For detailed information the install.txt.

## Program usage
### Server Program options
Please see the description in the top of the sketch and read the documentation (odt)

### client Program options
start ./amdtc --help-all AND read the documentation (odt)

## Versioning

### version 1.0 / February 2020
 * Initial version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgements

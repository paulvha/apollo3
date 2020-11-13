# Exchange data over BLE between Artemis/Apollo3 and linux

## ===========================================================

A solution with server software running on an Apollo3 board and client
software running on Linux (tested Raspberry PI or Ubuntu 18.04)

## Getting Started
As part of a project to better understand Bluetooth, I have worked
connecting boards from Sparkfun based on Apollo3 over Bluetooth
with Raspberry Pi and Ubuntu.
The connection is using the lightweight, propriety AMD Transaction Protocol. (AMDTP)
See the extras folder for more information about this protocol.

For detailed information about the solution, please read the amdtp.odt in the extras folder.

## Prerequisites
### Server :
The server software for the Apollo3 is available on https://github.com/paulvha/apollo3/tree/master/ble_amdtp_arduino
Also read the odt in that extras directory

If you plan to add an BME280 on the server
<br> BME280   : https://github.com/sparkfun/SparkFun_BME280_Arduino_Library

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
### version 2.0.1 / November 2020
 * fixed missing handles and variables
 * fixed install.txt parameters
 * Tested and warning Bluez 5.55

### version 2.0 / March 2020
 * updated to handle hash
 * updated float values batterylevel and temperature
 * added client and server version number
 * updated documentation

### version 1.1 / February 2020
 * updated timer handling to prevent deadloop

### version 1.0 / February 2020
 * Initial version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgements

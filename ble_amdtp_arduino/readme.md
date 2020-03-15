# Exchange data over BLE between two Artemis/Apollo3  boards

## ===========================================================

A solution with server software running on an Apollo3 board and client
another Apollo3 board. Tested with arduino IDE 1.8.12

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
Make sure to have at least version 1.0.30 for the Apollo3 Arduino software

### Client
Make sure to have at least version 1.0.30 for the Apollo3 Arduino software

## Software installation
please read the amdtp-arduino.odt in the extras folder.

## Program usage
### Server Program options
Please see the description in the top of the sketch and read the documentation (odt)

### client Program options
Please see the description in the top of the sketch and read the documentation (odt)

## Versioning

### version 1.0 / March 2020
 * Initial version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgements
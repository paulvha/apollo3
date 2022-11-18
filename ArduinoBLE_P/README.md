# BLE on Sparkfun Library V1.2.3 for Apollo3 /Artemis

## ===========================================================

## Background

Sparkfun has 2 library versions for the Artemis / Apollo3 boards.
The V1.2.3 and ![V2.2.1](https://github.com/sparkfun/Arduino_Apollo3). V2.2.1 was the latest at time of writing.
<br> Only the V2 version seems to be maintained and this is build on top off / with MBED-OS. It supports ArduinoBLE and
looks to be the future for the Apollo3 boards.

<br>There are many programs that still use V1.2.3. The code can still be installed within the Arduino IDE. The binary code that is created is much lighter, smaller easier to understand. BUT V1.2.3 does not have BLE. The BLE stack that was available in earlier V1 versions has been removed.

<br>This is a ArduinoBLE_P package contains the necessary BLE-stack files for V1.2.3 and ArduinoBLE version 1.3.2 taken from ![ArduinoBLE](https://github.com/arduino-libraries/ArduinoBLE).
A number of known bugfixes have been applied. It is delivered as is and there are NO plans to incorporate new functionalities or apply bug-changes as they become known or available. A number of bugs are known to be solved in future versions of the official version, see their ![website](https://github.com/arduino-libraries/ArduinoBLE/issues).

## Getting started
Make sure to install V1.2.3 for Artemis/Apollo3 Sparkfun boards in the Arduino IDE.

In case you have the official ArduinoBLE installed, it will not work without modification on V1.2.3. No need to change or remove as with
this package an ArduinoBLE_P will be installed that has been tested and works.

<br>Otherwise no special library dependencies.

## Software installation
For the Arduino examples : follow carefully the steps defined file : install_instructions.
In case you also want the Ubuntu examples, see the README in the subfolder Ubuntu.

## Versioning

### version 1.1.0 / November 2022
 * added peripheral and central example11 and example12
 * added central examples running on Ubuntu/Linux for example10, example11 and example12

### version 1.0.1 / October 2022
 * updated files for Sparkfun library V1.2.3
 * updated to use ArduinoBLE 1.3.2 as starting point
 * applied different bug fixes (see extras folder)
 * updated peripheral and central examples for BME280
 * added BME280 solution that can run on Linux/ubuntu

### version 1.0 / April 2021
 * Initial version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License

### The ArduinoBLE software is licensed under :

![license](https://github.com/arduino-libraries/ArduinoBLE/blob/master/LICENSE)

Copyright (c) 2019 Arduino SA. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

### The Sparkfun BLE driver is licensed under :
![license](https://github.com/sparkfun/Arduino_Apollo3/blob/master/docs/LICENSE.md)

SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).

### added or change code
Parts of the code written by me that has been added / changed to enable ArduinoBLE on V1.2.3 are delivered without support and the same license applies as it is for the orginal ArduinoBLE or Sparkfun code.


# BLE on Sparkfun Library V1.2.1 for Apollo3

## ===========================================================

## Background

Sparkfun has 2 library versions for the Artemis / Apollo3 boards.
The V1.2.1 and ![V2.0.6](https://github.com/sparkfun/Arduino_Apollo3). 2.0.6 was the latest at time of writting.
<br> Only the V2 version seems to be maintained and this is build on top off / with MBED-OS. Is supports ArduinoBLE and
looks to be the future for the Apollo3 boards.

<br>There are many programs that still use V1.2.1. The code can still be installed within the
Arduino IDE. The binary code that is created is much lighter, smaller easier to understand. BUT V1.2.1 does not have BLE. The BLE
stack that was available in earlier V1 versions has been removed.

<br>This is a port of the necessary BLE-stack files for V1.2.1 and  ArduinoBLE version 1.13 taken from ![ArduinoBLE](https://github.com/arduino-libraries/ArduinoBLE).
A number of known bugfixes have been applied. It is delivered as is and there are NO plans to incorporate new functionalities or apply bug-changes as they become known or available. A number of bugs are known to be solved in future versions of the official version, see their ![website](https://github.com/arduino-libraries/ArduinoBLE/issues).

## Getting started
* Make sure to install V1.2.1 for Artemis/Apollo3 Sparkfun boards in the Arduino IDE.
* DO not INSTALL or remove the official ArduinoBLE library as a patched version will be installed

<br>Otherwise no special library dependencies.

## Software installation
Follow carefully the steps defined file : install_instructions

## Versioning

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

### Parts of the code written by me that has been added / changed to enable ArduinoBLE on V1.2.1 are delivered without support and
the same license applies as it is for the orginal ArduinoBLE or Sparkfun code.


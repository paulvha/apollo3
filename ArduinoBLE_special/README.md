# BLE SPECIAL on Sparkfun Library V1.2.3 for Apollo3 /Artemis

## ===========================================================

## Background

Sparkfun has 2 library versions for the Artemis / Apollo3 boards.
The V1.2.3 and ![V2.2.1](https://github.com/sparkfun/Arduino_Apollo3). V2.2.1 was the latest at time of writing.
<br> Only the V2 version seems to be maintained and this is build on top off / with MBED-OS. It supports ArduinoBLE and
looks to be the future for the Apollo3 boards.

<br>There are many programs that still use V1.2.3. The code can still be installed within the Arduino IDE. The binary code that is created is much lighter, smaller easier to understand. BUT V1.2.3 does not have BLE. The BLE stack that was available in earlier V1 versions has been removed.

<br>This is a ArduinoBLE_P package contains the necessary BLE-stack files for V1.2.3 and ArduinoBLE version 1.3.3 taken from ![ArduinoBLE](https://github.com/arduino-libraries/ArduinoBLE).
A number of known bugfixes have been applied. It is delivered as is and there are NO plans to incorporate new functionalities or apply bug-changes as they become known or available. A number of bugs are known to be solved in future versions of the official version, see their ![website](https://github.com/arduino-libraries/ArduinoBLE/issues).

<br> The ExactLE part is focussed on Apollo3 / Artemis. If you use ArduinoBLE (on the many Arduino or Sparkfun boards that do not rely on Apollo3 V1)
Most of the examples can be used for any ArduinoBLE implemenation.

## Getting started
In case of V1.2.3 for Artemis/Apollo3 Sparkfun boards in the Arduino IDE, make sure to install HCI connection layer.

In case you have the official ArduinoBLE installed. No need to change or remove as with this package an ArduinoBLE_P will be installed that has been tested and works.

<br>Otherwise no special library dependencies or see top of sketches

## Software installation
For the Arduino examples : follow carefully the steps defined file : install_instructions.
In case you also want the Ubuntu examples, see the README in the subfolder Ubuntu.

## Versioning
### Version 1.1.8 / February 0205
 # added example25 to set the BLE signal strength
 
### version 1.1.7 / March 2023
 * updated with ArduinoBLE 1.3.4

### version 1.1.6 / March 2023
 * updated with ArduinoBLE 1.3.3

### Version 1.1.5 / January 2023
 * Added deepsleep example24 (same as example21 but now BME280 with SPI communication)
 * Added example23 SPS30 peripheral, central and Android app)
 * Added iBeacon example22 peripheral and Beacon scanner central
 * Added deepsleep example21 peripheral and central
 * Removed most of the unnessary code for ExactLE. MAKE SURE to install the latest version !

### Version 1.1.4 / January 2023
 * Added source Android app for example20_ph_BME280

### Version 1.1.3 / December 2022
 * Added output/input_control peripheral and source Android app.

### Version 1.1.2 / December 2022
 * Fixed issue(s) with discover descriptors (https://github.com/arduino-libraries/ArduinoBLE/issues/277)

### version 1.1.1 / November 2022
 * Added peripheral and central example13 and example14
 * Added central examples running on Ubuntu/Linux for example13 and example14

### version 1.1.0 / November 2022
 * Added peripheral and central example11 and example12
 * Added central examples running on Ubuntu/Linux for example20, example11 and example12

### version 1.0.1 / October 2022
 * Updated files for Sparkfun library V1.2.3
 * Updated to use ArduinoBLE 1.3.2 as starting point
 * Applied different bug fixes (see extras folder)
 * Updated peripheral and central examples for BME280
 * Added BME280 solution that can run on Linux/ubuntu

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


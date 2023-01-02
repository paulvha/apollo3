# ArduinoBLE_P - November 2022: update and tested on V1.2.3

This is based on ArduinoBLE 1.3.2, but adjusted for Sparkfun library V1.2.3. Next to that a number of changes have been applied. (see doc folder). For installation instruction see Doc-folder.

## examples have been added with a menu structure:

### example20_ph_bme280 / example20_central_bme280
In this setup an BME280 temperature, humidity and pressure sensor is connected to the peripheral. On regular intervals  it will send updated information on notify characteristic. This central has a menu to request data now, change the parameters of the data and stop sending data.

### example11_ph_RW / example11_central_RW
In this setup there are 4 characteristic. Two String characteristics (one reading to and one reading from), and two characteristics (one reading to and one reading from) for binary data. The central can send and read, same for the peripheral.

### example12_ph_RW_Notify / example12_central_RW_Notify
In this setup a large data message is split in multiple blocks exchanged using a flow control. There are 3 characteristics: one for peripheral to send the large data message, one for the central to send commands and feedback and one for notify.
The notify is used by the peripheral to send command and feedback and as such also indicate to the central that a next block is ready to be read.

### exampe13_MTU_test peripheral sketch: example13_ph_MTU_size
This is using an enhancement in the ArduinoBLE_P to understand the impact of MTU size and how to read it.

### exampe14_RW_N_MTU peripheral sketch: example14_ph_RW_notify_MTU
This is the same as example12, but then using the agreed MTU as blocksize

Next to a central in the Arduino IDE environment there is also an central in the ubuntu/linux environment.

Also there are 2 Android apps available
BME280 : works with peripheral example20_ph_BME280
Input / output control which is a combination of an Android app and Sketch to control input/output.

[![Compile Examples Status](https://github.com/arduino-libraries/ArduinoBLE/workflows/Compile%20Examples/badge.svg)](https://github.com/arduino-libraries/ArduinoBLE/actions?workflow=Compile+Examples) [![Spell Check Status](https://github.com/arduino-libraries/ArduinoBLE/workflows/Spell%20Check/badge.svg)](https://github.com/arduino-libraries/ArduinoBLE/actions?workflow=Spell+Check)

Enables BLE connectivity on the Arduino MKR WiFi 1010, Arduino UNO WiFi Rev.2, Arduino Nano 33 IoT, and Arduino Nano 33 BLE.

This library supports creating a BLE peripheral and BLE central mode.

For the Arduino MKR WiFi 1010, Arduino UNO WiFi Rev.2, and Arduino Nano 33 IoT boards, it requires the NINA module to be running [Arduino NINA-W102 firmware](https://github.com/arduino/nina-fw) v1.2.0 or later.


For more information about this library please visit us at:
https://www.arduino.cc/en/Reference/ArduinoBLE

## License

```
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
```

# MBED BLE

## ===========================================================

As part of the MBED in V2 there are a large number of features included, one of which is
the CORDIO BLE stack. The ArduinoBLE library is not using anything of that, only the connection to the
HAL and HCI layer on the Apollo3 chip. In this library I experiment to get this work. First between 2 Artemis
boards (ATP and EDGE) and later also between a BLEAK client running on Ubuntu.

## Getting Started

It started off with experimenting with a server sketch it is only advertising. As part of that
advertising it is updating a simulated battery value. But this could also be a temperature or other sensor value.

The next step was a server and a client that can connect using the heartrate service, followed by an implementation
to send BME280 information. Next an implementation of the famous NUS (Nordic Uart Service). All these have to take
into account that a length of a single message is max 20 characters. The Cordio stack will not accept anything else.

<br>Including then the Ambiq Micro Data Transfer Protocol (AMDTP). This will allow a (default) maximum data packets of 512 bytes.
The data will be spilt into chunks by the sender and combined in the receiver. Either it is the client or server.
There is a complete document and flow charts how this is handled in the extras folder.
It is not fast... but it does work and for most sensors this is a good solution.
To demonstrate how to use AMDTP there are 2 sketches example 16 and example17.

All examples have a server / peripheral on Artemis, they have a client / central on Artemis AND.....
they have a client / Central that can runs on a Python library : [BLEAK](https://github.com/hbldh/bleak). BLEAK is able to run on both Windows and
Linux / Ubuntu. I have only tested on Ubuntu 20.04. You can now exchange sensor data to the PC. The python files are in the
library folder bleak-examples

## Prerequisites
On Ubuntu you will need to install the [BLEAK](https://github.com/hbldh/bleak) software. This is available for Windows and Linux
For the BME280 sketch the Sparkfun library is needed as defined in the sketch.

## Software installation
Obtain the zip and install like any other.

## Program usage
### Program options
Please see the description in the top of the sketches

## Versioning

### version 1.0 / February 2022
 * Initial version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgments
Ambiq Micro for the AMDTP documentation as included the SDK.
[MBED examples](https://os.mbed.com/docs/mbed-os/v6.7/apis/ble.html)



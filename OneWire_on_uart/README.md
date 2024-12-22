# 1-Wire on UART

## ===========================================================

1-Wire is a communication channel that is used for a number of devices like Maxim DS18x20 temperature sensors and ibuttons. There is well known and has a great library (https://github.com/PaulStoffregen/OneWire) that works on many boards without any problem.

This library also works well on a Sparkfun® Artemis / Apollo3 board with the Sparkfun® V1.x.x library BUT NOT with the V2.x.x library.

This provides an alternative solution, build within the original 2.3.6 library, by leveraging a UART to perform 1-Wire.


## Getting Started
Make sure to read the 1-wire_uart.odt document for detailed description.This document will explain the root cause for failure and provides an alternative solution by leveraging a UART to perform 1-Wire.

## Prerequisites
none

## Software installation
Obtain the zip and install like any other

## Program usage
Please see the description in the top of the sketch and read the documentation (odt)

## Versioning

### version 3.0.0 / December 2021
 * based on the original library 2.3.6

## Author
 * https://www.pjrc.com/teensy/td_libs_OneWire.html
 * UART addition Paul van Haastrecht (paulvha@hotmail.com)

## License
* As defined by the original library

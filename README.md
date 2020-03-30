# Artemis / Apollo3 examples

## ===========================================================

Different examples for the Apollo3  processor.

## Getting Started
In the different folders are different programs with a readme file

## Prerequisites
Some examples have an optional Bluetooth client that requires
Bluez bluetooth stack : http://www.bluez.org/download/

## Software installation
Obtain the zip and install like any other for the Arduino
CHeck the extras directory for client software information

## Program usage
### Program options
Please see the description in the top of the sketch
For the Linux client software use the --help or -h option

## Versioning

### version 1.0 / January 2020
 * Initial version tested with the edge board
 * btble : will read battery level, temperature and set battery load resistor
 * btble : contains an optional client that has been tested on Ubuntu and Raspberry Pi
 * btble : tested on the orginal version from Sparkfun voor Apollo3 (1.0.23)

### Version 1.0.1 / January 2020
 * added batt_temp tested on the edge board but should work on any Apollo3
 * batt_temp will read local battery level, temperature and set battery load resistor
 * batt_temp is NOT using the standard analogRead(), but intializes the ADC module it self

### Version 2.0 / March 2020
 * updated the BLE-amdtp between Apollo3 board and Linux
 * added BLE-amdtp between 2 Apollo3 boards
 * updated amdtps, amdtpc Apollo3 and amdtpc Linux
 * tested on Apollo3 version 1.0.30

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgements
Make sure to have the Apollo3 datasheet from https://cdn.sparkfun.com/assets/learn_tutorials/9/0/9/Apollo3_Blue_MCU_Data_Sheet_v0_9_1.pdf

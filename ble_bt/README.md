# Artemis / Apollo3 example

## ===========================================================

This is extended version of BLE_LED and it will read the battery level,
set the battery resistor and read temperature in celsius or Fahrenheit
to make this available over Bluetooth.

Apart from the instructions below for nRFConnect, a client program has been written
in C in combination with Bluez-5.52 Bleutooth stack for Linux. The program has been
tested on Ubuntu and Raspberry Pi. The instructions and source code can be found in
the extras folder, the btble directory.

## Versioning

### version 1.0 / January 2020
 * Initial version tested with the edge board
 * btble : will read battery level, temperature and set battery load resistor
 * btble : contains an optional client that has been tested on Ubuntu and Raspberry Pi
 * btble : tested on the orginal version from Sparkfun voor Apollo3 (1.0.23)

### version 1.0.1 / January 2020
 * changed update of service values
 * changed UUID for battery load resistor to 128 bits

### version 1.0.2. / february 2020
 * change timer handling

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgements
Make sure to have the Apollo3 datasheet from https://cdn.sparkfun.com/assets/learn_tutorials/9/0/9/Apollo3_Blue_MCU_Data_Sheet_v0_9_1.pdf

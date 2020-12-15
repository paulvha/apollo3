An example for the AMD transfer protocol. Will enable data communication of
different kinds over bluetooth between an Artemis board as server and a linux client

# Arduino IDE/ Apollo3

Copy the 'amdtp_server' directory example into the Arduino IDE. (tested with version 1.8.13)
Make sure to use Apollo3 Sparkfun Library (at least) 2.0.3
Set parameters and if necessary install BME280. (see arduino_amdtp.odt)
compile as normal, load to the board and start the interactive monitor.

# Client side : Ubuntu or Raspberry Pi

## Install Bluez

Make sure to use Bluez 5.55 (there have been code changes since  Bluez-5.52
which cause a crash)

Follow the instruction for Bluez installation and compilation on:
https://www.jaredwolff.com/get-started-with-bluetooth-low-energy
P.s. there are many other sites that provide guidance as well.

For Raspberry Pi there are extended instructions, but the one above should do
https://learn.adafruit.com/install-bluez-on-the-raspberry-pi/installation

## Install amdtc client
Once you have installed and compiled Bluez, then :

cd bluez-5.55
copy the 'amdtc' directory as a directory in this bluez directory
cd amdtc

optional : update permissions : chmod +x make_amdtc
compile : ./make_amdtc

# Versioning

## paulvha / December 2020 / Version 3.1
 * update to work with Bluez 5.55 (does not work with Bluez 5.52)
 * improvements for stability

## paulvha / October 2020 / Version 3.0
 * Based on the complete change in version 2.0.1 changes have been made to work with the new amdtp-server

## paulvha / February 2020 / Version 1.0
 * initial version

# To run

to execute type :
./amdtc -b 56:77:88:23:AB:EF -I (with the correct destination address) to
get the interactive menu.

or ./amdtc --help   for Bluetooth options / help information

or ./amdtc --help-cmd for the user interface option with the ble_amdts server

P.s. to get an BLuetooth address run : hcitool lescan

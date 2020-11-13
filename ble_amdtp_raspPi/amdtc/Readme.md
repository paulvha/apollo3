An example for the AMD transfer protocol. Will enable data communication of
different kinds over bluetooth between an Artemis board as server and a linux clien

paulvha / February 2020 / Version 1.0

# Arduino IDE/ Apollo3

Copy the 'ble_amdts' directory example into the Arduino IDE.
compile as normal, load to the board and start the interactive monitor.

# Client side Ubuntu or Raspberry Pi

## Install Bluez
Follow the instruction for Bluez installation and compilation on:
https://www.jaredwolff.com/get-started-with-bluetooth-low-energy
P.s. there are many other sites that provide guidance as well.

For Raspberry Pi there are extended instructions, but the one above should do
https://learn.adafruit.com/install-bluez-on-the-raspberry-pi/installation

## Install amdtc client
Once you have installed and compiled Bluez, then :

cd bluez-5.55 (or what your version might be)
copy the 'amdtc'-directory as a directory in this bluez directory
cd btble

optional : update permissions : chmod +x make_BTBLE
compile : ./make_amdtc

to execute type :
./amdtc -b 56:77:88:23:AB:EF -I (with the correct destination address) to
get the interactive menu.

or ./amdtc --help   for Bluetooth options / help information

or ./amdtc --help-cmd for the user interface option with the ble_amdts server

P.s. to get an BLuetooth address run : hcitool lescan

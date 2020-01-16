An example to read the battery value, set battery resistor load and
read the internal temperature over Bluetooth

paulvha / January 2020 / Version 1.0

# Arduino IDE/ Apollo3

Copy the 'ble_bt' directory example into the Arduino IDE.
compile as normal, load to the board and start the interactive monitor.

# Client side

You have 3 options

## Mobile phone

Install the nRF Connect app on your mobile device (must support BLE bluetooth)
you can now connect and read the battery value and internal temperature .
This option will allow you to check all the available services.

## Ubuntu and Raspberry Pi

### Install Bluez
Follow the instruction for Bluez installation and compilation on:
https://www.jaredwolff.com/get-started-with-bluetooth-low-energy
P.s. there are many other sites that provide guidance as well.

### GATTTOOL
With the gatttool, part of Bluez, you can see all the available services.
The battery client (below) is based on the gatttool, but does not require
the gatttool is. However it is not always compiled and installed if you do want
it. On the version I had (5.52) I had to add --enable-deprecated.

./configure --enable-library --enable-deprecated

For Raspberry Pi there are extended instructions, but the one above should do
https://learn.adafruit.com/install-bluez-on-the-raspberry-pi/installation

### Install battery / temperature  client
Once you have installed and compiled Bluez, then :

cd bluez-5.52 (or what your version might be)
copy the 'btble'-directory as a directory in this bluez directory
cd btble

optional : update permissions : chmod +x make_BTBLE
compile : ./make_BTBLE

to execute type :
./btble -s -b 56:77:88:23:AB:EF (with the correct destination address) to
get the percentage power supply.

or ./btble --help   for more options / help information

P.s. to get an BLuetooth address run : hcitool lescan

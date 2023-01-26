# central examples met libgatt

These examples have been tested on Ubuntu 22.04, but are expected to work on other platforms as well.

## Prerequisite

The examples are build using libgatt. I have taken a version from [https://github.com/NaeemKK/libgatt](https://github.com/NaeemKK/libgatt).
I have included the examples I created in this version and that complete package is 'libgatt+examples.zip'.
The libgatt-master.zip is the original download.

The different examples have been tested against peripheral-sketches that are created using ArduinoBLE.
Installing and using peripheral-sketches with Arduino IDE are not part of this description.
In order to test you need to have an Arduino IDE, ArduinoBLE. The CPU board must have enough memory.
In my tests I used Sparkfun Artemis ATP, Sparkfun Artemis Edge and Sparkfun Micromod with nRF52480.

## Examples
The following examples were added to work as central:

### example23_SPS30		     peripheral sketch: example23_ph_sps30_BLE
In this setup an SPS30 is connected to the peripheral. On regular intervals it will send updated information on notify characteristic. This central has a menu to request data now, set clean, sleep and wakeup and stop sending data.

### output_input_control   peripheral sketch: output_input_sketch
Central to control the Arduino on-board led, 2 output pins, 1 digital input, 1 analog input and simulated battery

### example20_BME280		   peripheral sketch: example20_ph_bme280
In this setup an BME280 temperature, humidity and pressure sensor is connected to the peripheral. On regular intervals it will send updated information on notify characteristic. This central has a menu to request data now, change the parameters of the data and stop sending data.

### exampe14_RW_N_MTU      peripheral sketch: example14_ph_RW_notify_MTU
This is the same as example12, but then using the agreed MTU as blocksize. Only works with ArduinoBLE_P.

### exampe13_MTU_test      peripheral sketch: example13_ph_MTU_size
This is using an enhancement in the ArduinoBLE_P to understand the impact of MTU size and how to read it.

### example12_RW_N         peripheral sketch: example12_ph_RW_Notify
In this setup a large data message is split in multiple blocks exchanged using a flow control. There are 3 characteristics: one for peripheral to send the large data message, one for the central to send commands and feedback and one for notify.
The notify is used by the peripheral to send command and feedback and as such also indicate to the central that a next block is ready to be read.

### example11_RW           peripheral sketch: example11_ph_RW
In this setup there are 4 characteristics. Two String characteristics (one reading to and one reading from), and two characteristics (one reading to and one reading from) for binary data. The central can send and read, same for the peripheral.

In the top of each of the files and sketches there are more details. Also each of the examples have command line options.

For each of the peripheral sketches above there are also example-central-sketches based on ArduinoBLE
In my tests I used Sparkfun Artemis ATP, Sparkfun Artemis Edge and Sparkfun Micromod with nRF52480.

## Installing
Parts of this has been taken from the README.md that are part of the aforementioned libgatt.

Build libgatt
=============

* libgatt requires the following packages: `libbluetooth-dev`, `libreadline-dev`.
On Debian based system (such as Ubuntu), you can installed these packages with the
following command: `sudo apt-install libbluetooth-dev libreadline-dev`

* Unzip the attached complete 'libgatt+examples' in a folder.
```
cd <libgatt-src-root>
mkdir build && cd build
cmake ..
make
```
* The executables are now in a subfolder of the build-folder.

* The sources that you can read and edit are subfolders in :
```
cd <libgatt-src-root>
cd examples
```

* After you made a change and want to recompile :
```
cd <libgatt-src-root>
cd build
make
```

## Adding other examples
In order to add your own examples you have to make a number of adjustments. In the root-directory of libgatt there is a detailed description in the file add_example.txt

## versioning
 * version 1.0 / November 2022 / paulvha

## remarks
I have not tested with the original source : https://github.com/labapart/gattlib.


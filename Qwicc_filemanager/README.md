# QWICC Openlog filemanager for reading and writing
## ===========================================================

## Background
Wanted to created a sketch to handle the Sparkfun Qwicc Openlog module (DEV-15164) for reading and writing.
It has been on library version V1.2.3 as well as V2.2.0. No change needed. But I would expect this to run on other boards as well. The V1.1 has been adjusted and tested with an ATMEGA as well. While this could also work on an UNO, the memory-size is causing limitations.

<br> The Qwicc Openlog module is able to be connected with Wire from different boards to log data on a MicroSD card.
You can access the MicroSD directly, but you need to use the Sparkfun library that communicates with the board. This filemanager is making use of that library.

## Usage
* Adjust optionally the variables are line 40 in the sketch
* select the right board and bootloader in the Arduino IDE
* compile and upload
* open the Serial monitor with 115200

It will show :

<br>QWICC OpenLog filemanager. (V1.1)
<br>Press Enter to continue.
<br>
<br>
<br>1  = list the files in current directory
<br>2  = Change directory
<br>3  = Create new directory
<br>4  = Create new file
<br>5  = Back to root directory
<br>6  = Remove file
<br>7  = Remove directory
<br>8  = One level up to parent directory
<br>9  = Read file. Display in ASCII only
<br>10 = Read file. Display in HEX and ASCII
<br>11 = Add content to file
<br>15 = Close SD
<br>Your input (? = help)

<br>Instead of a number you can also do the menu selection as:
<br>
<br>ls = list
<br>cd = change directory
<br>ro = return to root
<br>rf = remove file
<br>rd = remove directory
<br>up = one level up
<br>ad = display ASCII
<br>hd = display hex and ASCII
<br>ap = append to file
<br>

NOTE 1:
Make sure to use a formatted MicroSD card.

NOTE 2:
It is a known issue that starting the SD card will take extra power. If after pressing <enter> the serial monitor becomes a little grey and the menu is not showing, the USB power was not sufficient enough. The board will be reset and probably another USB port has now been selected. Close the serial monitor, reset and /or remove the USB plug and try again. If it keeps failing try to add extra power (e.g. battery)

NOTE 3:
It is strongly adviced to close the SD BEFORE removing the power. The SD could get damaged otherwise.

NOTE 4:
The maximum file size to read is based on the buffer length. This can be set to maximum of 65535 due to library limitation.

## Dependency
The Sketch is depending on the [Sparkfun library]( https://github.com/sparkfun/SparkFun_Qwiic_OpenLog_Arduino_Library).

## Versioning

### Version 1.2 / September 2022
 * updated the approach to get the directory entries after discovering an issue in Artemis V2.xx Wire-library

### Version 1.1 / September 2022
 * A number of program and flow enhancements
 * Changes applied to make this also work when connected to an ATmega

### Version 1.0 / March 2022
 * initial version for Qwicc OpenLog

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgments
Part of the source code is based on the examples included in the original library

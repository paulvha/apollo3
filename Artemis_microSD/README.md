# MicroSD menu for reading and writing
## ===========================================================

## Background
Wanted to created a sketch to handle the Microsd reading and writting
It has been on library version V1.2.3 as well as V2.2.0. No change needed

<br> This MicroSD sketch has been developed to work on different Artemis platforms.
You need to select the right platform in the top of the sketch.

* MicroMod MainBoard (DEV-18576) with MM Artemins Processor (WRL-16781)
* MicroMod Data logger carrier board (DEV-16829) with MM Artemins Processor (WRL-16781)
* Artemis OpenLog (DEV-16832)
* Artemis ATP (DEV-15442) with external SparkFun Level Shifting microSD (DEV-13743)

## Usage
* select the right board in the top of sketch (around line 65)
* select the right board and bootloader in the Arduino IDE
* compile and upload
* open the Serial monitor

It will show :

<br> MicroSD menu example. Version 1.2
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
<br>8  = One level up
<br>9  = Read file. Display in ASCII only
<br>10 = Read file. Display in HEX and ASCII
<br>15 = Close SD

NOTE 1:
Make sure to use a formatted MicroSD card.

NOTE 2:
It is a known issue that starting the SD card will take extra power. If after pressing <enter>
the serial monitor becomes a little grey and the menu is not showing, the USB power was not sufficient
enough. The board will be reset and probably another USB port has now been selected. Close the serial
monitor, reset and /or remove the USB plug and try again. If it keeps failing try to add extra power (e.g. battery)

NOTE 3:
It is strongly adviced to close the SD BEFORE removing the power. The SD could get damaged otherwise.

## Versioning

### version 1.2 / February 2022
 * added support for Micromod datalogger & MicroMod main board with Artemis processor

### Version 1.1 / December 2021
 * added support for Artemis ATP

### Version 1.0 / December 2021
 * initial version for Artemis OpenLog

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgments
Part of the source code for the [Artemis Openlog](https://github.com/sparkfun/OpenLog_Artemis) has been included.

#  MicroSD filemanager for reading and writing
## ===========================================================

## Background
Wanted to created a sketch to handle the MicroSD reading and writing
For the Artemis , it has been tested on library version V1.2.3 as well as V2.2.0. No change needed.

<br> This filemanager sketch has been developed to work on different Artemis platforms and the Micromod nRF52840 & ESP32:
You need to select the right platform in the top of the sketch.

* MicroMod MainBoard (DEV-18576) with MicroMod Artemis Processor (WRL-16781)
* MicroMod Data logger carrier board (DEV-16829) with MicroMod Artemis Processor (WRL-16781)
* Artemis OpenLog (DEV-16832)
* Artemis ATP (DEV-15442) with external SparkFun Level Shifting microSD (DEV-13743)
* MicroMod MainBoard (DEV-18576) with MM nRF52840 Processor (WRL-16984)
* MicroMod Data logger carrier board (DEV-16829) with RF52840 Processor (WRL-16984)
* MicroMod MainBoard (DEV-18576) with MM ESP Processor (WRL-16781)
* MicroMod Data logger carrier board (DEV-16829) with MM ESP32 Processor (WRL-169781)
* MicroMod Input and Display carrier board (DEV-16985) with MM Artemis Processor (WRL-16401)
* MicroMod Input and Display carrier board (DEV-16985) with MM ESP Processor (WRL-16781)
* MicroMod Input and Display carrier board (DEV-16985) with MM RF52840 Processor (WRL-16984)

## Usage
* select the right board in the top of sketch (around line 134)
* select the right board and bootloader in the Arduino IDE
* compile and upload
* open the Serial monitor with 115200

It will show :

<br>Artemis MicroSD filemanager (Version 1.5)
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
<br>11 = Show hidden folder / files
<br>12 = Rename file
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
<br>rn = rename file
<br>

NOTE 1:
Make sure to use a formatted MicroSD card.

NOTE 2:
It is a known issue that starting the SD card will take extra power. If after pressing <enter> the serial monitor becomes a little grey and the menu is not showing, the USB power was not sufficient enough. The board will be reset and probably another USB port has now been selected. Close the serial monitor, reset and/or remove the USB plug and try again. If it keeps failing try to add extra power (e.g. battery)

NOTE 3:
It is strongly adviced to close the SD BEFORE removing the power. The SD could get damaged otherwise.

## Dependency
The sketch is depending on the SdFat v2.0.7 by Bill Greiman which can be installed through a link in the top of the sketch.

## Versioning

### Version 1.6 / March 2023
	* Added MicroMod Input and Display carrier board (DEV-16985) with MM Artemis Processor (WRL-16401)
	* Added MicroMod Input and Display carrier board (DEV-16985) with MM ESP Processor (WRL-16781)
	* Added MicroMod Input and Display carrier board (DEV-16985) with MM RF52840 Processor (WRL-16984)

### Version 1.5 / April 2022
 *  Added support for MM nRF52840 on MM Mainboard and MM Data logger carrier board.
 *  Added support for MM ESP32 on MM Mainboard and MM Data logger carrier board.

### version 1.4 / March 2022
 * Added abbreviation input posibility
 * Added rename file function
 * Improved HEX display
 * Fixed some typo's

### version 1.3 / February 2022
 * Added option 11 to show hidden folder / files
 * Fixed display size

### version 1.2 / February 2022
 * Added support for MicroMod datalogger & MicroMod main board with Artemis processor

### Version 1.1 / December 2021
 * Added support for Artemis ATP

### Version 1.0 / December 2021
 * Initial version for Artemis OpenLog

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgments
Part of the source code for the [Artemis Openlog](https://github.com/sparkfun/OpenLog_Artemis) has been included.

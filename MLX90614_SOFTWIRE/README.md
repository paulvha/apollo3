SparkFun MLX90614 Arduino Library
========================================

![SparkFun Infrared Thermometer Evaluation Board](https://cdn.sparkfun.com//assets/parts/5/6/4/6/10740-01a.jpg)

[*SparkFun Infrared Thermometer Evaluation Board (SEN-10740)*](https://www.sparkfun.com/products/10740)

An Arduino library that interfaces with the MLX90614 non-contact infrared thermometer over a 2-wire, I2C-like interface (SMBus). 

****************************************** 
Containing a special port with SOFTWIRE
Tested on Artemis ATP with V 1.2.3 and UNOR4
******************************************
The MLX90614 is using SMBUS for communication. It is nearly the same as I2C but different enough for MLX90614 to fail on Artemis/Apollo3 (and maybe others)<BR>
A document I2C_Apollo3.pdf with details can be found in the Document folder. As a first solution a PWM approach was done and while that worked the question whether a SoftWire approach could also solve this. <br>

The Softwire library has been taken from the ESP8266 and adjusted specifically for the MLX90614 library.  It is placed in the MLX90614 src subfolder SoftWire. <br>
In the file src/SoftWire/defwire.h you can select to either compile with the original Wire-library or the MLX-SOFTWIRE. <BR>
The original example1 - example4 have been changed and saved as example11 - example14 to work with the MLX-SOFTWIRE.<BR>
If you want to compile the original examples you have to the change in the file src/SoftWire/defwire.h to prevent compile errors.

It has been tested on an Artemis ATP with V1.2.3. The clock frequency is fixed and between 25 and 30KHZ. <br>
The minimum for SMbus is 10Khz and thus it fails on V2.2.1 as that was only 8Khz.<br>
On the UNOR4 Wifi the standard wire works with the MLX90614, but also the MLX-SOFTWIRE has been tested. The frequency is ~36Khz <br>

************************************************************************************************

Repository Contents
-------------------

* **/examples** - Example sketches for the library (.ino). Run these from the Arduino IDE.
* * **/src** - Source files for the library (.cpp, .h).
* **keywords.txt** - Keywords from this library that will be highlighted in the Arduino IDE.
* **library.properties** - General library properties for the Arduino package manager.

Documentation
--------------

* **[Installing an Arduino Library Guide](https://learn.sparkfun.com/tutorials/installing-an-arduino-library)** - Basic information on how to install an Arduino library.
* **[Product Repository](https://github.com/sparkfun/IR_Thermometer_Evaluation_Board-MLX90614)** - Main repository (including hardware files) for the Infrared Thermometer Evaluation Board.
* **[Hookup Guide](https://learn.sparkfun.com/tutorials/mlx90614-ir-thermometer-hookup-guide)** - Basic hookup guide for the Infrared Thermometer Evaluation Board.

Products that use this Library
---------------------------------

* [Infrared Thermometer Evaluation Board - MLX90614](https://www.sparkfun.com/products/10740)- A Arduino-compatible evaluation board for the MLX90614.
* [Infrared Thermometer - MLX90614 ](https://www.sparkfun.com/products/9570) -- The MLX90614 IC itself

Version History
---------------
* [V 1.0.0](https://github.com/sparkfun/SparkFun_MLX90614_Arduino_Library/tree/V_1.0.0) - Initial release

License Information
-------------------

This product is _**open source**_!

The **code** is released under the MIT license. See the included LICENSE file for more information.

Distributed as-is; no warranty is given.

- Your friends at SparkFun.

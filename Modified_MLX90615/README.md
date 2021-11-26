# Adjusted MLX90615 library to work on Sparkfun Artemis V1 library and others

This library will allow you to set the MLX90615 permanently in PWM mode and read either the object or Ambient temperature.
The libray is a modified version of the orginal, with the original references below.

The examples have all been updated and can be compiled on Sparkfun Artemis board , the V1.x library ONLY or for AVR boards

The following examples are available :
MLX_changeAddr   : change the MLX90615 I2C address
MLX_Set_Mode     : change MLX90615 between SMB and PWM mode
MLX_Check_Mode   : will try to connect and display in SMB and/or PWM mode
MLX_MultiDevice  : will demo to read multiple MLX90615 on single I2C channel
MLX_SingleDevice : will demo reading a single MLX90615 in SMB mode

Included is a document around the SMBUS compliance for Apollo3

## MLX_Set_Mode experience
To switch an MLX90615 between SMB and PWM is not a easy as one would expect. Creating a library and sketch to allow that brought a number of challenges, which I will try to share here.

According to the datasheet you need to set bit 0 in the config register to zero and power off /on to activate PWM :
1. The config register contains other information that should not be altered so the first step is to read the config register.
2. Then set bit 0 to PWM, Bit 1 for high or low frequency and Bit 2 to measure either the object of the ambient temperature.
3. Now erase the current config register with 0x0000, wait at least 5mS
4. Write the updated content and wait again at least 5mS
5. Power off / ON

### PWM starts automatically

One of the major challenge is timing. "officially" the config register is read after power-up, but it happened more than once that 5mS after erasing the config register the MLX started to provide PWM signals. Trying to write the updated content failed and the config register was left at zero’s.
This looks to be a problem with the firmware and the only way to get around was removing the power for a longer time and then it would wait for step 5. thus I changed the reboot time to 3 seconds instead of 1. Seemed to do the trick.

Next time around trying to switch between PWM and SMB fails as a read of the register would return zeros. Now your original content is lost. I was lucky to make a print first of the config content (0x144D) so I could restore. i have left that option in the code to enable

### T-Min incorrect

Also had issues with T-Min. By default that should be 0x3B, but my sensor it was 0x0. I have created a separate function that can be called to update T-Min and/or the address if you want that. Later discovered that the orginal change address was the root cause as it was handled wrongly. This version has been updated

### I2C address

Although not changing the I2C address when updating T_Min, also here after an erase it happened that I could not update. In case the I2C 0x0 is used on for an MLX it will ALWAYS respond (no matter what it's address is). I have left that in as an option to use, but make sure there are NO other sensors on the I2C channel as they might respond as well !!

===========================================================================================================



# Digital_Infrared_Temperature_Sensor_MLX90615  [![Build Status](https://travis-ci.com/AsharaStudios/Digital_Infrared_Temperature_Sensor_MLX90615.svg?branch=master)](https://travis-ci.com/AsharaStudios/Digital_Infrared_Temperature_Sensor_MLX90615)

<img src=https://statics3.seeedstudio.com/images/product/101020077%201.jpg width=400> <img src=https://statics3.seeedstudio.com/product/101020077%201_01.jpg width=400>

[Grove - Digital Infrared Temperature Sensor](https://www.seeedstudio.com/Grove-Digital-Infrared-Temperature-Sensor-p-2385.html)

This Grove - Digital Infrared Temperature Sensor is baseed on MLX90615 which is a high resolution infra Red thermometer for noncontact temperature measurements,readout resolution is 0.02 Celcius.Factory calibrated in wide temperature range: -40 ~ 85 cilcius for sensor temperarure and -40 ~ 115 cilcius for object temperature.The thermometer is factory calibrated with the digital SMBus compatible interface enabled,but the our Arduino/Seeeduino library is based on a soft i2c library,you can use any pins on any AVR chip to drive the SDA and SCL lines.

More detail refer to [seeed wiki](http://wiki.seeedstudio.com/Grove-Digital_Infrared_Temperature_Sensor/)

## Usage

1. Clone this repo or download as a zip;
2. Unzip the zip file if you downloaded a zip file;
3. Copy the directory "Digital_Infrared_Temperature_Sensor_MLX90615" into Arduino's libraries directory;
4. Open Arduino IDE, go to "File"->"Examples"->"Digital_Infrared_Temperature_Sensor_MLX90615"

----

This software is written for [Seeed Technology Inc.](http://www.seeed.cc) and is licensed under [The MIT License](http://opensource.org/licenses/mit-license.php). Check License.txt/LICENSE for the details of MIT license.

Contributing to this software is warmly welcomed. You can do this basically by
[forking](https://help.github.com/articles/fork-a-repo), committing modifications and then [pulling requests](https://help.github.com/articles/using-pull-requests) (follow the links above
for operating guide). Adding change log and your contact into file header is encouraged.
Thanks for your contribution.

Seeed is a hardware innovation platform for makers to grow inspirations into differentiating products. By working closely with technology providers of all scale, Seeed provides accessible technologies with quality, speed and supply chain knowledge. When prototypes are ready to iterate, Seeed helps productize 1 to 1,000 pcs using in-house engineering, supply chain management and agile manufacture forces. Seeed also team up with incubators, Chinese tech ecosystem, investors and distribution channels to portal Maker startups beyond.

[![Analytics](https://ga-beacon.appspot.com/UA-46589105-3/Digital_Infrared_Temperature_Sensor_MLX90615)](https://github.com/igrigorik/ga-beacon)

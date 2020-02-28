/*
 * paulvha / January 2020 / version 1.0
 *
 * paulvha/ February 2020 / version 1.0.1
 * # update to timer handling
 * 
 *  This is extended version of BLE_LED and it will read the battery level, 
 *  set the battery resistor and read temperature in celsius or Fahrenheit 
 *  to make this available over Bluetooth.
 *  
 *  Apart from the instructions below for nRFConnect, a client program has been written 
 *  in C in combination with Bluez-5.52 Bleutooth stack for Linux. The program has been
 *  tested on Ubuntu and Raspberry Pi. The instructions and source code can be found in 
 *  the extras folder, the btble directory
 *
 *  IMPORTANT.  Your use of this file is governed by a Software License Agreement
 *  ("Agreement") that must be accepted in order to download or otherwise receive a
 *  copy of this file.  You may not use or copy this file for any purpose other than
 *  as described in the Agreement.  If you do not agree to all of the terms of the
 *  Agreement do not use this file and delete all copies in your possession or control;
 *  if you do not have a copy of the Agreement, you must contact ARM Ltd. prior
 *  to any use, copying or further distribution of this software.
 
  Below the original header
  
  **************************************************************************************
    
  Analog to digital conversion
  By: Owen Lyke
  SparkFun Electronics 
  Date: Aug 27, 2019
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  This example demonstrates basic BLE server (peripheral) functionality for the Apollo3 boards.

  How to use this example:
    - Install the nRF Connect app on your mobile device (must support BLE bluetooth)
    - Make sure you select the correct board definition from Tools-->Board 
      (it is used to determine which pin controls the LED)
    - Compile and upload the example to your board with the Arduino "Upload" button
    - In the nRF Connect app look for the device in the "scan" tab. 
        (By default it is called "Artemis BLE" but you can change that below)
    - Once the device is found click "connect"
    - The GATT server will load with 5 services:
      - Generic Access
      - Generic Attribute
      - Link Loss
      - Immediate Alert
      - Tx Power

    - For this example we've hooked into the 'Immediate Alert' service. 
        You can click on that pane and it will expand to show an "upload"  button.
        Use the upload button to write one of three values (0x00, 0x01, or 0x02)
    - When you send '0x00' (aka 'No alert') the LED will be set to off
    - When you send either '0x01' or '0x02' the LED will be set to on
*/

#include "BLE_example.h"

#define BLE_PERIPHERAL_NAME "Artemis BLE" // Up to 29 characters

void setup() {
  
  #ifdef BLE_Debug  // defined in ble_debug.h
    SERIAL_PORT.begin(115200);
    delay(1000);
    SERIAL_PORT.printf("Apollo3 Arduino BLE Battery & Temperature Example. Compiled: %s\n", __TIME__);
  #endif

  //
  // Configure the peripheral's advertised name: (tag_main.c)
  setAdvName(BLE_PERIPHERAL_NAME);

  //
  // Boot the radio.
  //
  HciDrvRadioBoot(0);

  //
  // Initialize the main ExactLE stack.
  //
  exactle_stack_init();

  // init ADC reading (need to start with Temperature first as sometimes it does not provide the right information
  // known issue in 1.0.29 requested to be fixed by Sparkfun development in later version
  read_adc(2);
  
  //
  // Start the "Tag" profile.
  //
  TagStart();
}

void loop() {
      
      //
      // Calculate the elapsed time from our free-running timer, and update
      // the software timers in the WSF scheduler.
      //
      update_scheduler_timers();
      wsfOsDispatcher();
}

//*****************************************************************************
//
//! @file batt_temp.ino
//!
//! @brief Example of ADC sampling VBATT voltage divider, BATT load, and temperature.
//!
//! version 1.0 / January 2020 / paulvha
//*****************************************************************************

/******************************************************************************
 * This example is using the complex way to get these values as it will initialize
 * and handle the ADC registers itself, instead of relying on the Arduino Apollo3
 * library to handle and perform analogRead() calls. 
 * 
 * This was the result of a deep study to understand how the ADC implementation of
 * the Apollo3 was working to get a better understanding of the ADC module in the Apollo chip.
 * 
 * In the included Readme I try to explain what is happening
 */
//*****************************************************************************
//
// Copyright (c) 2020, Paulvha
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

#include "adc_batt_temp.h"

//
// set to 1 to enable debug messages in the sketch
//
#define B_DEBUG 0

//
// Trigger load resistor on / off in loop()
//
bool batt_load=false;
bool prev_batt_load = batt_load;

void setup() {

  Serial.begin(115200);
  delay(1000);
  Serial.printf("ADC VBATT and Temperature Sensing Example. Compiled: %s\n", __TIME__);

  //
  // Initialize ADC port for Battery and Temperature reading
  //
  adc_init();

  //
  // print status at start
  //
  if (B_DEBUG) print_state();
}

void loop() {
    //
    // enable / disable load resistor each loop
    //
    if (batt_load)  {
      Serial.println("\n ****** With Resistor load ******\n");
      
      // only perform when it changed
      if (prev_batt_load != batt_load)  battery_load(true);
    }
    else {
      Serial.println("\n ****** NO load ******\n");
      
      // only perform when it changed
      if (prev_batt_load != batt_load)  battery_load(false);
    }

    // remember last battery load
    prev_batt_load = batt_load;
    
    // toggle load resistor on/off for next loop. You could/should see a small difference
    // in battery level. If large... your battery is getting weak.
    // by commenting out the line below this option is disabled.
    batt_load =! batt_load;

    //
    // obtain and display battery level
    //
    float fVBATT = get_batt_value();

    if (fVBATT == 0)  Serial.println("Could not obtain the Battery level\n");
    else 
      Serial.printf("Battery level : %2.2f Volt, Percentage %2.2f %%\n", fVBATT, fVBATT / 3.3);

    //
    // obtain and display temperature
    //
    float fADCTempDegreesC = get_temp_value(true);

    if (fADCTempDegreesC == 0)  Serial.println("Could not obtain the Temperature\n");
    else 
      Serial.printf("Temperature Celsius : %2.2f C, Fahrenheit %2.2f F\n", 
                              fADCTempDegreesC, (fADCTempDegreesC * (180.0f / 100.0f)) + 32.0f);
    // wait for 2 seconds
    delay(2000);
}

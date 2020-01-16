
//*****************************************************************************
//
//! @file adc_batt_temp.h
//!
//! @brief ADC sampling VBATT voltage divider and temperature  & setting Battery load resistor.
//!
//! version 1.0 / January 2020 / paulvha
//*****************************************************************************

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

#ifndef _BT_MEAS_H_
#define _BT_MEAS_H_

#include "Arduino.h"

//*****************************************************************************
//
// Forward declarations.
//
//*****************************************************************************
//
// initialize the ADC for reading. Must be called as part of setup() in the sketch
//
void adc_init();

//
// prints the ADC and trim values (for debug only)
//
void print_state();

//
// Obtain the current battery level
//
float get_batt_value();

//
// obtain the current temperature from the Apollo3 chip
// @param celsius : 
//       true  : return temperature in celsius, 
//       false : return temperature in Fahrenheit
//
float get_temp_value(bool celsius);

//
// add load resistor (~500 ohm) to check battery level under load conditions with a next get_batt_value()
// @param act : 
//   true  : set load resistor, 
//   false : remove load resistor
//
// NOTE : make sure to turn this off after testing to preserve battery  !!!
//
void battery_load(bool act);

#endif // _BT_MEAS_H_

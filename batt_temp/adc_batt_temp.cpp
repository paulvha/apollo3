//*****************************************************************************
//
//! @file adc_batt_temp.cpp
//!
//! @brief ADC sampling VBATT voltage divider and temperature  & setting Battery load resistor.
//!
//! Purpose: This example initializes the ADC, provide 5 user level calls (see .h file):
//!
//! void adc_init();
//! float get_batt_value();
//! float get_temp_value(bool celsius);
//! void battery_load(bool act);
//! void print_state();
//!
//! version 1.0 / January 2020 / paulvha
//
// This is in part part based on
//  adc_Vbatt : revision v2.2.0-7-g63f7c2ba1 of the AmbiqSuite Development Package.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2020, Ambiq Micro + Paulvha
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

//
//*****************************************************************************

#include "adc_batt_temp.h"

//*****************************************************************************
//
// Global Variables
//
//*****************************************************************************
//
//  Reference Voltage for ADC compare
//  set in am_hal_adc_config_t g_sADC_Cfg as AM_HAL_ADC_REFSEL_INT_2PO,
//
const float fReferenceVoltage = 2.0;

//
// ADC code for voltage divider base level.
//
uint16_t g_ui16ADCVDD_code = 0;

//
// ADC code for temperature sensor base level.
//
uint16_t g_ui16ADCTEMP_code = 0;

//
// enable or disable debug info for the driver
//
#define BT_DEBUG 0


// define the sample size for a slot
// only after the number of samples have passed, the FIFO will be updated
// Valid Sample Size:         PERFORM_SAMPLE
//  AM_HAL_ADC_SLOT_AVG_1,    1
//  AM_HAL_ADC_SLOT_AVG_2,    2
//  AM_HAL_ADC_SLOT_AVG_4,    4
//  AM_HAL_ADC_SLOT_AVG_8,    8
//  AM_HAL_ADC_SLOT_AVG_16,   16
//  AM_HAL_ADC_SLOT_AVG_32,   32
//  AM_HAL_ADC_SLOT_AVG_64,   64
//  AM_HAL_ADC_SLOT_AVG_128   128
//
// you MUST equal the PERFORM_SAMPLE with the number of samples
//
#define SLOT_SAMPLE    AM_HAL_ADC_SLOT_AVG_2
#define PERFORM_SAMPLE 2

//*****************************************************************************
//
// set ADC Configuration
//
//*****************************************************************************
const static am_hal_adc_config_t g_sADC_Cfg =
{
    //
    // Select the ADC Clock source.
    //
    .eClock = AM_HAL_ADC_CLKSEL_HFRC_DIV2,

    //
    // Polarity (trigger)
    //
    .ePolarity = AM_HAL_ADC_TRIGPOL_RISING,

    //
    // Select the ADC trigger source using a trigger source macro.
    //
    .eTrigger = AM_HAL_ADC_TRIGSEL_SOFTWARE,

    //
    // Select the ADC reference voltage.
    //
    .eReference = AM_HAL_ADC_REFSEL_INT_2P0,        // internal 2V
    .eClockMode = AM_HAL_ADC_CLKMODE_LOW_POWER,     // Disable the clock between scans for LPMODE0.

    //
    // Choose the power mode for the ADC's idle state.
    // AM_HAL_ADC_LPMODE1:
    //
    // Powers down all circuity and clocks associated with the
    // ADC until the next trigger event. Between scans, the reference
    // buffer requires up to 50us of delay from a scan trigger event
    // before the conversion will commence while operating in this mode.
    //
    .ePowerMode = AM_HAL_ADC_LPMODE1,

    //
    // Enable single shot update of ADC.
    //
    .eRepeat = AM_HAL_ADC_SINGLE_SCAN
};

//*****************************************************************************
//
// Read the Battery and Temperature ADC values
//
// Success : g_ui16ADCVDD_code or g_ui16ADCTEMP_code should NOT be 0
//
//*****************************************************************************

void adc_read()
{
  uint32_t ui32NumSamples, ui32IntMask;
  am_hal_adc_sample_t sSample;

  // reset values
  g_ui16ADCVDD_code = g_ui16ADCTEMP_code = 0;

  // as we set for X samples in the slot configuration
  // we have to trigger the X times to read sample
  // before it will show up in the FIFO
  uint8_t i;
  for ( i=0 ; i < PERFORM_SAMPLE; i++){
    
    // a delay improves the quality of the output
    delay(200);   
    
    //
    // Kick Start ADC software trigger .
    //
    am_hal_adc_sw_trigger(g_ADCHandle);
  }
  
  //
  // Wait for complete interrupt
  //
  while (1)
  {
    // Read the interrupt status.
    if (AM_HAL_STATUS_SUCCESS != am_hal_adc_interrupt_status(g_ADCHandle, &ui32IntMask, false))
    {
        Serial.println("Error reading ADC interrupt status");
        return;
    }
    if (ui32IntMask & AM_HAL_ADC_INT_CNVCMP)
        break;
  }
  
  //
  // Emtpy the FIFO, we'll just look at the last one read.
  // only slot 6 and 7 are enabled in adc_init() and will provide output
  //
  while ( AM_HAL_ADC_FIFO_COUNT(ADC->FIFO) )
  {
    ui32NumSamples = 1;

    // false : only read the ADC value, NOT the full sample with the 6 extra-bits
    am_hal_adc_samples_read(g_ADCHandle, false, NULL, &ui32NumSamples, &sSample);

    //
    // Determine which slot it came from (see adc_init for slot assignments)
    //
    if (sSample.ui32Slot == 6 )
    {
      //
      // The returned ADC sample is for the battery voltage divider.
      //
      g_ui16ADCVDD_code = sSample.ui32Sample;

    }
    else
    {
      //
      // The returned ADC sample is for the temperature sensor.
      //
      g_ui16ADCTEMP_code = sSample.ui32Sample;
    }
  }
}

//*****************************************************************************
//
// obtain Battery value
//
// return 0 = error
//
//*****************************************************************************
float get_batt_value()
{
  // read the ADC value
  adc_read();

  if(BT_DEBUG) {
    Serial.printf("Battery ADC dec.%d, 0x%X\n", g_ui16ADCVDD_code*1000/16384, g_ui16ADCVDD_code );
  }

  // no value received
  if (g_ui16ADCVDD_code == 0)  return(0);

  //
  // Compute the voltage divider output.
  // The battery level is divided by 3 (DIV3) as such we need to multiple by 3 to get the actual level
  // The reference voltage is in 16384 steps as ADC is set for 14 bit in adc_init()
  //
  return ((float)g_ui16ADCVDD_code) * 3.0f * fReferenceVoltage / 16384.0f;
}

//*****************************************************************************
//
// Enable or disable internal load resistor to test the battery
//
// @param act
//  true  : set resistor load
//  false : remove  resistor load
//
//*****************************************************************************
void battery_load(bool act)
{
  if (act)  MCUCTRL->ADCBATTLOAD_b.BATTLOAD = 1;
  else MCUCTRL->ADCBATTLOAD_b.BATTLOAD = 0;

  // dummy read to flush anything in FIFO
  adc_read();
}

//*****************************************************************************
//
// obtain Temperature value
//
// return 0 = error
//
// @param celsius : 
//    true = celsius, 
//    false = Fahrenheit
//
//*****************************************************************************
float get_temp_value(bool celsius)
{
  float fADCTempVolts;
  float fVT[3];
  uint32_t ui32Retval;

  // read the ADC value
  adc_read();
  
  if(BT_DEBUG) {
    Serial.printf("Temperature ADC dec. %d, 0x%X\n",g_ui16ADCTEMP_code,g_ui16ADCTEMP_code);
  }
  
  // no value received
  if (g_ui16ADCTEMP_code == 0) return(0);

  //
  // Convert and scale the temperature.
  // Temperatures are in Fahrenheit range -40 to 225 degrees.
  // Voltage range is 0.825V to 1.283V
  // First get the ADC voltage corresponding to temperature. (14 bit ADC)
  //
  fADCTempVolts = ((float)g_ui16ADCTEMP_code) * fReferenceVoltage / 16384.0f;

  //
  // Now call the HAL routine to convert volts to degrees Celsius.
  // This will include TRIM (a correction value to the enable better ADC reading). 
  // Trim can be really measured/obtained for this chip or default values. Use print_stat() to tell
  //
  // It seems that default values is what normally is the case and then the results might be off.
  // According to the source: Since the device has not been calibrated on the tester, we'll load
  // default calibration values. These default values should result
  // in worst-case temperature measurements of +-6 degress C.
  //
  // I have 2 edge modules and had them run for an hour in a loop before taking these values: 
  // Measured with external temperature meter next tot the edge board it is around 20.5 C.
  // one module gives 16.5 C  - 17.5 C
  // the other gives  19.5 C - 20 C
  //
  fVT[0] = fADCTempVolts;
  fVT[1] = 0.0f;
  fVT[2] = -123.456;      // MUST be set to trigger conversion
  ui32Retval = am_hal_adc_control(g_ADCHandle, AM_HAL_ADC_REQ_TEMP_CELSIUS_GET, fVT);

  if ( ui32Retval == AM_HAL_STATUS_SUCCESS )
  {
      if (celsius) return(fVT[1]);
      else return((fVT[1] * (180.0f / 100.0f)) + 32.0f);
  }

  Serial.printf("Error: am_haL_adc_control returned %d\n", ui32Retval);
  return(0);
}

//*****************************************************************************
//
// ADC INIT Function
//
//*****************************************************************************
void adc_init(void)
{
    am_hal_adc_slot_config_t sSlotCfg;

    //
    // Initialize the ADC, the TRIM and get the handle.
    //
    if ( AM_HAL_STATUS_SUCCESS != am_hal_adc_initialize(0, &g_ADCHandle) )
    {
        Serial.printf("Error - reservation of the ADC instance failed.\n");
    }

    //
    // Power on the ADC.
    //
    if (AM_HAL_STATUS_SUCCESS != am_hal_adc_power_control(g_ADCHandle,
                                                          AM_HAL_SYSCTRL_WAKE,
                                                          false) )
    {
        Serial.printf("Error - ADC power on failed.\n");
    }

    //
    // Configure the ADC.
    //
    if ( am_hal_adc_configure(g_ADCHandle, (am_hal_adc_config_t*)&g_sADC_Cfg) != AM_HAL_STATUS_SUCCESS )
    {
        Serial.printf("Error - configuring ADC failed.\n");
    }

    //
    // configure the slots
    //
    sSlotCfg.bEnabled       = false;                        // disable slot from FIFO output
    sSlotCfg.bWindowCompare = true;
    sSlotCfg.eChannel       = AM_HAL_ADC_SLOT_CHSEL_SE0;    // Single-ended channels to Slot 0
    sSlotCfg.eMeasToAvg     = AM_HAL_ADC_SLOT_AVG_1;        // ADC measurement averaging configuration
    sSlotCfg.ePrecisionMode = AM_HAL_ADC_SLOT_14BIT;        // 0

    am_hal_adc_configure_slot(g_ADCHandle, 0, &sSlotCfg);   // Unused slot
    am_hal_adc_configure_slot(g_ADCHandle, 1, &sSlotCfg);   // Unused slot
    am_hal_adc_configure_slot(g_ADCHandle, 2, &sSlotCfg);   // Unused slot
    am_hal_adc_configure_slot(g_ADCHandle, 3, &sSlotCfg);   // Unused slot
    am_hal_adc_configure_slot(g_ADCHandle, 4, &sSlotCfg);   // Unused slot
    am_hal_adc_configure_slot(g_ADCHandle, 5, &sSlotCfg);   // Unused slot

    sSlotCfg.bEnabled       = true;                         // enable slot in FIFO output
    sSlotCfg.bWindowCompare = false;                        // no compare / trigger upper or lower limits
    sSlotCfg.eChannel       = AM_HAL_ADC_SLOT_CHSEL_BATT;
    sSlotCfg.eMeasToAvg     = SLOT_SAMPLE;                  // do X samples
    sSlotCfg.ePrecisionMode = AM_HAL_ADC_SLOT_14BIT;        // divide reference voltage 16384
    am_hal_adc_configure_slot(g_ADCHandle, 6, &sSlotCfg);   // BATT in SLOT 6

    sSlotCfg.bEnabled       = true;                         // enable slot in FIFO output
    sSlotCfg.bWindowCompare = false;                        // no compare / trigger upper or lower limits          
    sSlotCfg.eChannel       = AM_HAL_ADC_SLOT_CHSEL_TEMP;
    sSlotCfg.eMeasToAvg     = SLOT_SAMPLE;                  // do X samples
    sSlotCfg.ePrecisionMode = AM_HAL_ADC_SLOT_14BIT;        // divide reference voltage 16384
    am_hal_adc_configure_slot(g_ADCHandle, 7, &sSlotCfg);   // Temperature in SLOT 7

    //
    // Enable the ADC.
    //
    am_hal_adc_enable(g_ADCHandle);
}

//*****************************************************************************
//
// Print the current state of ADC. (for debug only)
//
//*****************************************************************************
void print_state()
{
    float fTrims[4];
    bool  bMeasured;

    //
    // Kick Start ADC 
    //
    adc_read();

    //
    // Print out ADC initial configuration register state.
    //
    Serial.printf("\n");
    Serial.printf("ADC REGISTERS @ 0x%08X\n", (uint32_t)REG_ADC_BASEADDR);
    Serial.printf("ADC CFG   = 0x%08X\n", ADC->CFG);
    Serial.printf("ADC SLOT0 = 0x%08X\n", ADC->SL0CFG);
    Serial.printf("ADC SLOT1 = 0x%08X\n", ADC->SL1CFG);
    Serial.printf("ADC SLOT2 = 0x%08X\n", ADC->SL2CFG);
    Serial.printf("ADC SLOT3 = 0x%08X\n", ADC->SL3CFG);
    Serial.printf("ADC SLOT4 = 0x%08X\n", ADC->SL4CFG);
    Serial.printf("ADC SLOT5 = 0x%08X\n", ADC->SL5CFG);
    Serial.printf("ADC SLOT6 = 0x%08X\n", ADC->SL6CFG);
    Serial.printf("ADC SLOT7 = 0x%08X\n", ADC->SL7CFG);

    //
    // Print out the temperature trim values as recorded in OTP.
    // Trim is a correction on ADC misreadings. This can either be set to default error-correction
    // or real performed calibration. It is automatically handled when calculating temperature
    // with am_hal_adc_control()
    //
    fTrims[0] = fTrims[1] = fTrims[2] = 0.0F;
    fTrims[3] = -123.456f;
    am_hal_adc_control(g_ADCHandle, AM_HAL_ADC_REQ_TEMP_TRIMS_GET, fTrims);
    bMeasured = fTrims[3] ? true : false;
    Serial.printf("\n");
    Serial.printf("TRIMMED TEMP    = %.3f\n", fTrims[0]);
    Serial.printf("TRIMMED VOLTAGE = %.3f\n", fTrims[1]);
    Serial.printf("TRIMMED Offset  = %.3f\n", fTrims[2]);
    Serial.printf("Note - these trim values are '%s' values.\n",
                         bMeasured ? "calibrated" : "uncalibrated default");
    Serial.printf("\n");
}

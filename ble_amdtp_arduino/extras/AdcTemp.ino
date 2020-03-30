/*  
 *  Program to determine the adjustment / offset for the Apollo3 temperature sensor.
 *   
 *  The Apollo3 internal temperature sensor can be measured with the ADC. However BOTH the sensor as well as the ADC
 *  circuit have percentage of error.  The Apollo3 can be factory calibrated to read the results +/- 3 degrees. If
 *  not calibrated SDK has build-in default adjustments values that can result is in +/- 6 degrees.
 *  Tested on 3 different boards and compared to BME280 : 
 *    ATP  board was calibrated. The difference is < 0.5 degrees.
 *    Edge board 1 around 1 degree, 
 *    Edge board 2 around 5 degrees.
 *   
 *  While all within 6 degrees, it might not be as good for you. 
 *   
 *  ONLY FOR NON-CALIBRATED BOARDS !!
 *  An alternative approach is developed that you CAN use in case the apollo3 is not calibrated. 
 *  In that case this program will use an BME280 or BME680 to determine the expected ADC 
 *  voltage. It will calculate the difference / offset to be applied. You can test your adjustment at
 *  the line 57 below, before applying to AMDTPS
 *   
 *  In AMDTPS you can decide whether to use this alternative approach by applying a correction that is different than zero.
 *  Best to have the environment on a stable temperature or about 25 degrees. 
 *   
 *  BME680 / 
 *  BME280      QWUIC
 *  GND         GND
 *  VCC         3V3
 *  SCK         SCL
 *  SDI         SDA
 *  
 *  The BME280 requires :https://github.com/sparkfun/SparkFun_BME280_Arduino_Library
 *  The BME680 requires: https://github.com/adafruit/Adafruit_BME680

 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 * 
 *  paulvha / March 2020 / version 1.0
 *   * initial version
 */
//****************************************************
//**     select only ONE external sensor to use     **
//****************************************************
#define BME_280
//#define BME_680

//*****************************************************
// set the offset to apply to test adjusted temperature
//*****************************************************
float TempOffset = -0.1234;

//***********************************************
//**                                           **
//**  NO CHANGES NEEDED BEYOND THIS POINT      **
//**                                           **
//***********************************************

#if (defined BME_280) && (defined BME_680)
#error SELECT ONLY ONE SENSOR
#endif

#if (not defined BME_280) && (not defined BME_680)
#error SELECT A SENSOR
#endif
 
#include <Wire.h>

#ifdef BME_280
#include "SparkFunBME280.h"
BME280 mySensor;
char sensor[]="BME280";
#endif

#ifdef BME_680 
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
Adafruit_BME680 mySensor; 
char sensor[]="BME680";
#endif

// trim values
bool  bMeasured;
float fCalibrationTemperature;
float fCalibrationVoltage;
float fCalibrationOffset;

void setup() {

  Serial.begin(115200);
  delay(1000);
  Serial.printf("Temperature correction detection. Compiled: %s\n", __TIME__);

  Wire.begin();
#ifdef BME_280
  if (! mySensor.beginI2C() ) //Begin communication over I2C
  {
    Serial.println("The BME280 did not respond. Please check wiring.");
    while(1);
  }
#endif

#ifdef BME_680
  if (!mySensor.begin()) {
    Serial.println("Could not find a valid BME680 sensor.Please check wiring!");
    while (1);
  }

  // Set up oversampling and filter initialization
  mySensor.setTemperatureOversampling(BME680_OS_8X);
#endif

  // obtain ADC information
  if (GetAdc() == false)
  {
    Serial.println("Failed to get ADC. on hold/r");
    while(1);
  }

  analogReadResolution(14);     // 14 bits ADC
}

void loop() {
  
#ifdef BME_680  
  if (! mySensor.performReading()) {
    Serial.println("Failed to perform reading BME680:(");
    return;
  }
#endif

  // if calibrated Apollo3 on the board
  if (bMeasured) display_cal();
  else display_alt();

  delay(1000);
}

// display information in case the Apollo3 has calibrated internal temperature sensor data

void display_cal()
{
  static bool header = true;
  
  float tempadc = analogRead(ADC_INTERNAL_TEMP);               // get temperature ADC reading
  float fADCTempVolts = ((float) tempadc)  * 2.0F / 16384.0F;  // to internal voltage measured 16384
  float itt = SDK_method(fADCTempVolts);                       // to temperature
#ifdef BME_280
  float Tbme = mySensor.readTempC();                           // read the external temperature sensor
#endif
#ifdef BME_680
   float Tbme = mySensor.temperature;
#endif
  if (header) {
    Serial.printf("\n------ Internal sensor----- -------%s -------\r\n", sensor);
    Serial.printf("       Temperature              Temperature\r\n");
    header = false;
  }
  
  Serial.printf("\t%2.2f\t\t\t %2.2f\t\t\tCALIBRATED APOLLO3\r\n", itt, Tbme);
}

// Display information in case the Apollo3 does NOT have calibrated internal temperature sensor data
// and is using default numbers.
// It will estimate the correction needed based on the measured temperature from BME280 or BME680 reading.
// The user can apply this adjustment manually (line 57), re-comppile and see the impact
// based on the information the user can decide :
// let amdpts program use the original method with default values OR use the alternative (manual) adjusted method.

void display_alt()
{
  static bool header = true;

  float tempadc = analogRead(ADC_INTERNAL_TEMP);               // read temperature ADC value
  float fADCTempVolts = ((float) tempadc)  * 2.0F / 16384.0F;  // to internal voltage measured 
  float itt = SDK_method(fADCTempVolts);                       // to temperature apply the original method

#ifdef BME_280
  float Tbme = mySensor.readTempC();                           // read the BME280 
#endif

#ifdef BME_680
   float Tbme = mySensor.temperature;                          // read the BME680
#endif

  float fTemp = (Tbme+273.15) * 3.8f /1000;                    // calculate the assumed internal voltage
  float fdelta = fADCTempVolts - fTemp;          // calculate the delta between measure and calculated internal voltage
  float utt = ((fADCTempVolts - TempOffset) * 1000 / 3.8F) - 273.15F; // calculate temperature using alternative method

  if (header) {
    Serial.printf("------ Internal sensor ----- -------%s-------  --- adjustment---    ----- adjusted ------ \r\n",sensor);
    Serial.printf("  org,Temp      voltage       Temperature  voltage      calculated       applied        Temperature\r\n");
    header = false;
  }
  Serial.printf("  %2.2f   \t%1.4f\t\t%2.2f\t   %1.4f\t", itt,fADCTempVolts,Tbme, fTemp);
  Serial.printf("%1.4f\t\t%1.4f\t\t%2.2f\r\n",fdelta, TempOffset, utt);
} 

// Calculate the temperature based on the trim in the Apollo3 library
// The trim is either calibrated or based on default values
// The Original way ! (from am_hol_analog.c)

float SDK_method(float fADCTempVolts)
{
  float itt  = fCalibrationTemperature;
  itt /= (fCalibrationVoltage - fCalibrationOffset);
  itt *= (fADCTempVolts - fCalibrationOffset);

  return (itt -273.15);   // return Celsius
}

// get adc information

bool GetAdc()
{
  float fTrims[4];
  void *ADCHandle;
  
  // There can only be 1 handle (as there is only one ADC unit) active at the same time.
  // When performing readAnalog(), the library will take ownership and will 
  // not let go the ADC unit. So first obtain the information
  
  //
  // Initialize the ADC, the TRIM and get the handle.
  //
  if ( AM_HAL_STATUS_SUCCESS != am_hal_adc_initialize(0, &ADCHandle) )
  {
    Serial.printf("Error - reservation of the ADC instance failed.\n");
    return(false);
  }

  //
  // Print out the temperature trim values as recorded.
  // Trim is a correction on ADC misreadings. This can either be set to default error-correction
  // or real performed calibration. 
  //
  fTrims[0] = fTrims[1] = fTrims[2] = 0.0F;
  fTrims[3] = -123.456f; // MUST be set to trigger info
  if (AM_HAL_STATUS_SUCCESS != am_hal_adc_control(ADCHandle,AM_HAL_ADC_REQ_TEMP_TRIMS_GET, fTrims))
  { 
    Serial.printf("Error - Could not read trim.\n");
    return(false);
  }  

  //
  // release the ADC unit
  //
  if ( AM_HAL_STATUS_SUCCESS != am_hal_adc_deinitialize(ADCHandle) )
  {
    Serial.printf("Error - releasing of the ADC instance failed.\n");
    return(false);
  }
  
  fCalibrationTemperature = fTrims[0];
  fCalibrationVoltage    = fTrims[1];
  fCalibrationOffset   = fTrims[2];
  bMeasured = fTrims[3] ? true : false;
  
  Serial.printf("\n");
  Serial.printf("TRIMMED TEMP    = %.6f\r\n", fTrims[0]);
  Serial.printf("TRIMMED VOLTAGE = %.6f\r\n", fTrims[1]);
  Serial.printf("TRIMMED Offset  = %.6f\r\n", fTrims[2]);
  Serial.printf("Note - these trim values are '%s' values.\r\n\n",
                       bMeasured ? "calibrated" : "uncalibrated default");

  if (bMeasured){
    Serial.printf("NO FURTHER ACTION NEEDED ON THIS BOARD\r\n");
    Serial.printf("This Apollo3 chip has been calibrated!\r\n");
  }
  
  return(true);
}

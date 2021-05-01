/*
  Get basic environmental readings from the BME280
  By: Nathan Seidle
  SparkFun Electronics
  Date: March 9th, 2018
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14348 - Qwiic Combo Board
  https://www.sparkfun.com/products/13676 - BME280 Breakout Board
  https://www.sparkfun.com/search/results?term=artemis  Artemis/Apollo3 board

  Adjusted by : Paul van Haastrecht
  Date: May 1, 2021

  This example shows :
  Use the Power Switch to enable power to a BME280
  how to read humidity, pressure, and current temperature from the BME280 over I2C.

  Hardware connections:
  BME280 -> ATP
  GND ->    GND
  3.3 ->    PAD / PIN 3
  SDA ->    SDA
  SCL ->    SCL

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <Wire.h>

#include "SparkFunBME280.h"
BME280 mySensor;

#include "APgpio.h"
// define the pad to use
uint32_t  padVDD = 3;     // ONLY pads 3 or 36

void setup()
{
  Serial.begin(115200);

  do { delay(500); } while(!Serial);

  Serial.println("Example4 : PowerSwitch & Reading basic values from BME280");

  // set pin as power switch to 3V3
  if (APmode(padVDD, M_VDD)) {
    Serial.printf("Pad %d has been set with PowerSwitch to 3V3\n", padVDD);

    if (APset(padVDD, HIGH)) Serial.printf("Pad %d (VDD) has been set HIGH\n", padVDD);
    else {
      Serial.printf("Pad %d (VDD) could not be set HIGH\n", padVDD);
      while(1);
    }

    Serial.printf("pad %d (VDD) level reads as %s.\n",padVDD,APread(padVDD)? "high":"low");
  }
  else {
    Serial.printf("Pad %d could not be set with PowerSwitch to 3V3\n", padVDD);
    while(1);
  }

  Wire.begin();

  if (mySensor.beginI2C() == false) //Begin communication over I2C
  {
    Serial.println("The sensor did not respond. Please check wiring.");
    while(1); //Freeze
  }
}

void loop()
{
  Serial.print("Humidity: ");
  Serial.print(mySensor.readFloatHumidity(), 0);

  Serial.print(" Pressure: ");
  Serial.print(mySensor.readFloatPressure(), 0);

  Serial.print(" Alt: ");
  //Serial.print(mySensor.readFloatAltitudeMeters(), 1);
  Serial.print(mySensor.readFloatAltitudeFeet(), 1);

  Serial.print(" Temp: ");
  //Serial.print(mySensor.readTempC(), 2);
  Serial.print(mySensor.readTempF(), 2);

  Serial.println();

  delay(50);
}

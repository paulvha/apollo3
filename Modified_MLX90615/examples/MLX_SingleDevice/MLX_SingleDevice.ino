/*
 * Read single 90615 in SMB mode
 * 
 *   Connect as follows
 *   
 *   SDA    SDA_PIN
 *   SCL    SCL_PIN
 *   VCC    PWR_PIN or 3V3
 *   GND    GND
 */
 
#include "MLX90615_m.h"

// set the address you expect the MLX is
// if 0x0 MAKE SURE ONLY ONE MLX IS ON THE i2c CHANNEL AS ALL DEVICES WILL RESPOND.
#define MLX MLX90615_DefaultAddr

// indicate this sketch will be on an V1 library
#define APOLLO3_V1

#ifdef APOLLO3_V1
// You can decide with IOM (and thus pins) to use by selecting an IOM. The corresponding pins are below.
//
//       SDA   SCL
// IOM 0  6    5   // default used by SPI, but you can overrule to use as WIRE
// IOM 1  9    8
// IOM 2  25   27
// IOM 3  43   42
// IOM 4  40   39  // default WIRE
// IOM 5  49   48

#define PWR_PIN 17 // power pin (set to 0 -zero- if direct connected to 3v3)

// choice the IOM
TwoWire myWire(0);  // in this case use IOM 0 and thus SDA on pin 6, SCL on pin 5.

// OR you can use the standard wire (which is IOM 4)
//TwoWire myWire = Wire;

#else

#define PWR_PIN 5    // power pin (set to 0 -zero- if direct connected to 3v3)
TwoWire myWire = Wire;

#endif

MLX90615 mlx90615(MLX, &myWire);

void setup() {
  Serial.begin(115200);
  while (!Serial); // Only for native USB serial
  delay(1000);     // Additional delay to allow open the terminal to see setup() messages
  Serial.println("MLX90615 Single Device SMB...");

 // enable power
  if(PWR_PIN != 0) {
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite (PWR_PIN, HIGH);

    // allow delay to start
    delay(1000);
  }

#ifdef APOLLO3_V1
  myWire.setClock(400000);  // set clock speed to 400Khz
#endif

  myWire.begin();
  delay(500);

  // // write data into EEPROM when you need to adjust emissivity.
  // Serial.println(mlx90615.writeReg(MLX90615_EEPROM_EMISSIVITY,0x0000)); // Erase! (and see result)
  // delay(10); // EEPROM Write/Erase time
  // Serial.println(mlx90615.writeReg(MLX90615_EEPROM_EMISSIVITY,Default_Emissivity)); // Desired
  // delay(10); // EEPROM Write/Erase time
  // mlx90615.readEEPROM(); //read EEPROM data to check whether it's a default one.
}

void loop() {
  Serial.print("Object temperature: ");
  Serial.println(mlx90615.getTemperature(MLX90615_OBJECT_TEMPERATURE));
  Serial.print("Ambient temperature: ");
  Serial.println(mlx90615.getTemperature(MLX90615_AMBIENT_TEMPERATURE));
  delay(1000);
}

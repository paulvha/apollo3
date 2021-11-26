/**
    This example covers the following options to communicate
    with additional MLX90615:

    Using a single bus for all MLX90615, only if each MLX
    has an unique address among the same I2C bus. Note that
    this sketch doesn't cover the address changing! See
    "changeAddr" for this.
*/

#include "MLX90615_m.h"

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

#define PWR_PIN 5    // power pin
TwoWire myWire = Wire;

#endif

#define DEVICE_ADDR1 0x5B
#define DEVICE_ADDR2 0x5C
#define DEVICE_ADDR3 0x5D

MLX90615 mlx90615_1(DEVICE_ADDR1, &myWire);
MLX90615 mlx90615_2(DEVICE_ADDR2, &myWire);
MLX90615 mlx90615_3(DEVICE_ADDR3, &myWire);

void setup() {
  Serial.begin(115200);
  while (!Serial);           // Only for native USB serial
  delay(1000);               // Additional delay to allow open the terminal to see setup() messages
  Serial.println("MLX 90615 MultiDevice...");

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
}

void loop() {
  float temperatureObj1 = mlx90615_1.getTemperature(MLX90615_OBJECT_TEMPERATURE);
  float temperatureObj2 = mlx90615_2.getTemperature(MLX90615_OBJECT_TEMPERATURE);
  float temperatureObj3 = mlx90615_3.getTemperature(MLX90615_OBJECT_TEMPERATURE);
  float temperatureAmb1 = mlx90615_1.getTemperature(MLX90615_AMBIENT_TEMPERATURE);
  float temperatureAmb2 = mlx90615_2.getTemperature(MLX90615_AMBIENT_TEMPERATURE);
  float temperatureAmb3 = mlx90615_3.getTemperature(MLX90615_AMBIENT_TEMPERATURE);

  Serial.print("Temp_1: ");
  Serial.print(temperatureObj1);
  Serial.print("°C  ");
  Serial.print(temperatureAmb1);
  Serial.println("°C  ");

  Serial.print("Temp_2: ");
  Serial.print(temperatureObj2);
  Serial.print("°C  ");
  Serial.print(temperatureAmb2);
  Serial.println("°C  ");

  Serial.print("Temp_3: ");
  Serial.print(temperatureObj3);
  Serial.print("°C  ");
  Serial.print(temperatureAmb3);
  Serial.println("°C  ");

  Serial.println("\n=======================================\n\r");

  delay(1000);
}

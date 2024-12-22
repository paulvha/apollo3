/**
    Be 200% Sure of that the only I2C device connected is a MLX90615!

    The idea is to address first to 0x00 (every MLX90615 --and maybe
    anyother I2C device-- will answer to this address), then update
    the EEPROM address 0x00 (register 0x10) by clearing and setting
    the new address, and finally, addressing with this new I2C address


  Connect as follows

  SDA    SDA_PIN
  SCL    SCL_PIN
  VCC    PWR_PIN or 3V3
  GND    GND
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

#define PWR_PIN 5    // power pin (set to 0 -zero- if direct connected to 3v3)
TwoWire myWire = Wire;

#endif

// use address 0x0 so the MLX will always respond
MLX90615 mlx90615(0x00, &myWire);

#define CUSTOM_ADDR 0x5D

void setup() {

  Serial.begin(115200);
  while (!Serial); // Only for native USB serial
  delay(1000);     // Additional delay to allow open the terminal to see setup() messages
  Serial.println("MLX90615. Change Address...");

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

  uint16_t val;
  
  // 1. Read the address and T-Min
  int result = mlx90615.readReg(MLX90615_EEPROM_SA, &val);
  if (result != 0) print_error(result);

  //val = 0x355B;      // comment out to restore default
  val &= 0xFF00;       // remove address but keep T_min
  val |= CUSTOM_ADDR;

  // 2. Change Addr & T-Min
  UpdateRegister(MLX90615_EEPROM_SA, val);

  Serial.print("new address : ");
  mlx90615.readReg(MLX90615_EEPROM_SA, &val);
  Serial.println(val & 0xff, HEX);   //read EEPROM data to check whether it's a default one.

  // Invoke the destructor
  mlx90615.~MLX90615();

  //2. Use NEW Addr
  MLX90615 mlx90615(CUSTOM_ADDR, &myWire);
}

void loop() {
  Serial.print("Object temperature: ");
  Serial.println(mlx90615.getTemperature(MLX90615_OBJECT_TEMPERATURE));
  Serial.print("Ambient temperature: ");
  Serial.println(mlx90615.getTemperature(MLX90615_AMBIENT_TEMPERATURE));

  delay(1000);
}

/**
 * update an EEPROM register on the MLX
 * @param reg : the MLX register
 * @param val : new 16 bit value
 */
void UpdateRegister(uint8_t reg, uint16_t val)
{
  int result = mlx90615.writeReg(reg, 0x0000); // Erase
  if (result != 0) print_error(result);
  delay(100); // EEPROM Write/Erase time

  result = mlx90615.writeReg(reg, val); // Write new registers
  if (result != 0) print_error(result);
  delay(1000); // EEPROM Write/Erase time
}

void print_error(int res)
{
  switch (res) {
    case -1:
      Serial.println("Bad CRC calc\r");
      break;
    case -2:
      Serial.println("I2C Error\r");
      break;
    case -10:
      Serial.println("I2C Connector not specified yet\r");
      break;
    default:
      Serial.print("Unknown error: ");
      Serial.println(res);
  }

  while(1);
}

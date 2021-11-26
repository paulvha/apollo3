/**
 * This sketch will check whether the MLX is in SMB or PWM mode
 * Version 1.0 / November 2021 / paulvha
 * 
 * 
 * Connect as follows
 * 
 * In order to only read in PWM :
 * 
 * SDA    PWM_PIN
 * SCL    SCL_PIN
 * VCC    PWR_PIN or 3V3
 * GND    GND
 * 
 * In order to only read in SMB :
 * 
 * SDA    SDA_PIN
 * SCL    SCL_PIN
 * VCC    PWR_PIN or 3V3 
 * GND    GND
 */
 
#include "MLX90615_m.h"

// set the address you expect the MLX is
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

#define SDA_PIN 6  // often referred to on the board as SPI CLK
#define SCL_PIN 5  // often referred to on the board as SPI MISO
#define PWR_PIN 17 // power pin (set to 0 -zero- if direct connected to 3v3)
#define PWM_PIN 15 // PWM reading

// choice the IOM
TwoWire myWire(0);  // in this case use IOM 0 and thus SDA on pin 6, SCL on pin 5.

// OR you can use the standard wire (which is IOM 4)
//TwoWire myWire = Wire;

#else

// Example for Uno
#define SDA_PIN A4 
#define SCL_PIN A5   
#define PWM_PIN 2    // only pin 2 or 3 can handle interrupt on UNO https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
#define PWR_PIN 5    // power pin (set to 0 -zero- if direct connected to 3v3)

TwoWire myWire = Wire;

#endif

bool ReadAsPWM;       // true is read PWM else read I2C

MLX90615 mlx90615(MLX, &myWire);
// In case you overwrite the MLX address and don't know to what,  use address 0x0
// !!!!!! make the MLX is the only device on the I2C channel !!!!
//MLX90615 mlx90615(0x0, &myWire);

void setup() {
  
  Serial.begin(115200);
  while (!Serial); // Only for native USB serial

  Serial.println("Check for SMB or PWM");

  // enable power
  if(PWR_PIN != 0) {
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite (PWR_PIN, HIGH);

    // allow delay to start
    delay(1000);
  }

  if ( ! CheckPWM()) {
    if (! CheckSMB()) {
      Serial.println("Could not detect MLX\r\nFreeze\r");
      while(1);
    }
  }
}

void loop() {
  if(ReadAsPWM) { // get as PWM
  
    Serial.print("PWM temperature: ");
    Serial.print(mlx90615.getPWMTemperature());
    Serial.println(" C\r");

    Serial.print("PWM temperature: ");
    Serial.print(mlx90615.getPWMTemperature(true));
    Serial.println(" F\r");
  
  }
  else {       // get as SMB
    
    Serial.print("Object temperature: ");
    Serial.println(mlx90615.getTemperature(MLX90615_OBJECT_TEMPERATURE));
    Serial.print("Ambient temperature: ");
    Serial.println(mlx90615.getTemperature(MLX90615_AMBIENT_TEMPERATURE));
  }
    
  delay(1000);

}

bool CheckPWM()
{
  uint8_t good = 0;
  ReadAsPWM = true;
  Init();

  for(uint8_t i =0 ; i < 10 ; i++)
  {
    float t = mlx90615.getPWMTemperature();

    if (t > 0 && t < 50) {
      Serial.print("PWM temperature: ");
      Serial.println(t);
      
      if(good == 0)
        Serial.println("Looks like it is running in PWM mode\r");
      
      good++;
    }
    else { 
      Serial.print("Looks like INCORRECT temperature in PWM mode :");
      Serial.println(t);
    }
  }
  
  if(good > 5) return true;
  else return false;
}

bool CheckSMB()
{
  uint8_t good = 0;
  ReadAsPWM = false;
  Init();
  
  for(uint8_t i = 0 ; i < 10 ;i++)
  {
    float t = mlx90615.getTemperature(MLX90615_OBJECT_TEMPERATURE);

    if (t > 0 && t < 50) {
      Serial.print("SMB Object temperature: ");
      Serial.println(t);
      
      if(good == 0)
        Serial.println("Looks like it is running in SMB mode\r");
      
      good++;
    }
    else{
      Serial.print("Looks like INCORRECT temperature in SMB mode :");
      Serial.println(t);
    }
  }
  if(good > 5) return true;
  else return false;
}

/* initialise the library */
void Init()
{
  if (ReadAsPWM) {
    myWire.end();
    
    // set SCL high during read
    pinMode(SCL_PIN, OUTPUT);
    digitalWrite(SCL_PIN, HIGH); 
  
    // initialise PWM
    mlx90615.pwm_init(PWM_PIN);
  
    Serial.print("For PWM reading : make sure the SDA-line of the MLX is connected to pin ");
    Serial.println(PWM_PIN);
  }
  else {
#ifdef APOLLO3_V1
    myWire.setClock(200000);  // set clock speed to 200Khz
#endif

    myWire.begin();
    
    Serial.println("Make sure the MLX is connected to SDA/SCL.");
  }

  // give time to get the first reading
  delay(1000); 
}

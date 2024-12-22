/* Version 1.0 / November 2021 / paulvha
 *  
 * This will enable changing an MLX9061x between PWM and SMBUS. The change will last after power
 * down, but can be reversed back with this sketch as well
 * 
 * The main reason is that SMBUS does not work on Apollo3 / Artemis running V2 library
 * Running on PWM might be a solution
 *  
 * You need to perform this sketch either on V1 / 200Khz or an other board (like an Arduino Uno)
 * If you run Apollo3 V1, you can select a different IOM for I2C of just use the Wire.
 * 
 * If you select another board, you need to update the SDA_PIN and SCL_PIN (see example for Uno)
 * 
 * Connect as follows
 * 
 * In order to SWITCH between PWM and SMB :
 * SDA    SDA_PIN
 * SCL    SCL_PIN
 * VCC    PWR_PIN
 * GND    GND
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
 * 
 */

#include "MLX90615_m.h"

// set the address you expect the MLX is
// if 0x0 MAKE SURE ONLY ONE MLX IS ON THE I2C CHANNEL AS ALL DEVICES WILL RESPOND.
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
#define PWR_PIN 5    // power pin

TwoWire myWire = Wire;

#endif

// PWM variables
#define PWM_FRQ HIGH  // set (high) 1kHZ, (low) 10Hz
#define PWM_TEMP HIGH // (HIGH) Objecttemp or (LOW) Ambient
bool ReadAsPWM;       // true is read PWM else read I2C

MLX90615 mlx90615(MLX, &myWire);
// In case you overwrite the MLX address and don't know to what,  use address 0x0
// !!!!!! make the MLX is the only device on the I2C channel !!!!
//MLX90615 mlx90615(0x0, &myWire);

void setup() {
  
  Serial.begin(115200);
  while (!Serial); // Only for native USB serial

  // enable power
  if(PWR_PIN != 0) {
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite (PWR_PIN, HIGH);

    // allow delay to start
    delay(1000);
  }
  
  //disable wire for now
  myWire.end();

#ifdef APOLLO3_V1
    myWire.setClock(400000);  // set clock speed to 400Khz
#endif

  char c = 'n';
  while (c == 'n' || c == 'N') {
    
    Serial.println("Want to change from SMB to PWM  (Y/N)?\r");
    c = ReadInput();
    if (c == 'y' || c == 'Y') {
      SMB_to_PWM();
      ReadAsPWM = true;
    }

    if (c == 'n' || c == 'N') {
      Serial.println("Want to change from PWM to SMB  (Y/N)?\r");
      c = ReadInput();
      
      if (c == 'y' || c == 'Y') {
        PWM_to_SMB();
        ReadAsPWM = false;
      }
    }
    
    if (c == 'n' || c == 'N') {  
      Serial.println("Want to start reading in PWM (Y/N)?\r");
      c = ReadInput();
      
      if (c == 'y' || c == 'Y') {
      ReadAsPWM = true;
      }
    }
    
    if (c == 'n' || c == 'N') {
      Serial.println("Want to start reading in SMB (Y/N)?\r");
      c = ReadInput();
      
      if (c == 'y' || c == 'Y') {
      ReadAsPWM = false;
      }
    }
  }

  // intitialise the library
  Init();
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
    myWire.begin();
    Serial.println("Make sure the MLX is connected to SDA/SCL.");
  }

  // give time to get the first reading
  delay(1000); 
}

/* read keyboard input */
char ReadInput(){
  
  //flush input
  while (Serial.available()) {
    Serial.read();
    delay(100);
  }
  
  // wait for input
  while(! Serial.available());
  
  // return input
  return(Serial.read());
}

/* set MLX permanently in SMB mode */
void PWM_to_SMB()
{
  if( SCL_PIN == 0 || PWR_PIN == 0){
      Serial.println("Can not perform switch, Missing Power and/or SCL pin\r");
      while(1);
  }

  Serial.print("** Make sure the MLX SDA line is connected to the SELECTED SDA pin ");
  Serial.print(SDA_PIN);
  Serial.println(" **\r");
  
  // Initiate the low pulse
  pinMode(SCL_PIN, OUTPUT);
  
  digitalWrite(SCL_PIN, LOW);
  
  // now delay for 100mS (although 39ms should do the trick)
  delay(100);

  // Stop the low pulse
  pinMode(SCL_PIN, INPUT);

  // make SMB permanent
  SMB_OR_PWM(true);
}

/* set MLX permanently in PWM mode */
void SMB_to_PWM()
{
  if (SCL_PIN == 0 || PWR_PIN == 0) {
    Serial.println("Can not perform switch, Missing Power and/or SCL pin\r");
    while(1);
  }
  
  SMB_OR_PWM(false);
}

/*
 * will set mode to SMB or PWM in config registers
 * param act : 
 *    true make SMB permanent 
 *    false make PWM permanent
 */
void SMB_OR_PWM(bool act)
{
  uint16_t val;

  myWire.begin();
  delay(500);
  
  //updateTmin();  // optional in case you need to update

  /*
  // display range only
  int result1 = mlx90615.readReg(MLX90615_EEPROM_PWMT_RNG, &val);
  if (result1 != 0) print_error(result1);
  Serial.print("MLX90615_EEPROM_PWMT_RNG  ");
  Serial.println(val,HEX);
  delay(100);
  */
  
  // read current config
  int result = mlx90615.readReg(MLX90615_EEPROM_CONFIG, &val);
  if (result != 0) print_error(result);

//Serial.print("val before ");
//Serial.println(val,HEX);

  // I got 0x14DD as a valid default number, BUT when I had SCL and SDA switched it was 0x5B
  // This is just an extra check to make sure have some reasonable before updating and writing back.
  val = 0x14dd;  // uncomment to reset to my default
  if (val < 0xfff || val == 0xffff) {
    Serial.print("Something went wrong in reading.\nGot current Config register as 0x");
    Serial.println(val, HEX);
    Serial.println("Expected something like 0x14dd. Check wiring and read in function SMB_OR_PWM\r");
    while(1);
  }
  
  if (act) {
    val |= 0x0001;               // set SMB bit          
  }
  else {
    uint16_t v = 0xfffe;         // set for PWM (bit 0 = 0)
    if (PWM_FRQ)  v &= 0xfffd;   // set for high frequency (bit 1 = 0)
    if (PWM_TEMP) v &= 0xfffB;   // set for object temperature (bit 2 = 0)

    val &= v;
  }

//Serial.print("val after update ");
//Serial.println(val,HEX); 

  UpdateRegister(MLX90615_EEPROM_CONFIG, val); // Write new config register

  delay(1000);
  
  myWire.end();

  // now reboot to enable new situation
  // ACTUALLY is looks that sometimes even without a reboot it moves from
  // SMB to PWM after 39mS ......
  reboot();

  if (act) {
    myWire.begin();
    delay(500);
  }
}

/* this will reset T_min and the I2C address.*/
void updateTmin()
{
  uint16_t val;
  int result = mlx90615.readReg(MLX90615_EEPROM_SA, &val);
  if (result != 0) print_error(result);
  
//Serial.print("MLX90615_EEPROM_SA  ");
//Serial.println(val,HEX);

  val &= 0x00ff;    // remove T_Min and keep I2C address

  val |= 0x3500;    // to keep the I2C address to what it was.
 // val |= 0x355B;    // OR set default T_Min (35) and default I2C address (5b)

//Serial.print("MLX90615_EEPROM_SA after update  ");
//Serial.println(val,HEX);

  UpdateRegister(MLX90615_EEPROM_SA, val); // Write new T_Min and address
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
  delay(30); // EEPROM Write/Erase time (5 according to datasheet)

  result = mlx90615.writeReg(reg, val); // Write new registers
  if (result != 0) print_error(result);
  delay(30); // EEPROM Write/Erase time (5 according to datasheet)
}

void reboot() {
  // toggle the power
  digitalWrite (PWR_PIN, LOW);
  delay(3000);
  digitalWrite (PWR_PIN, HIGH);

  // allow delay to start
  delay(1000);
}

void print_error(int res)
{
  switch (res) {
    case -1:
      Serial.println("Bad CRC calc\r");
      break;
    case -2:
      Serial.println("I2C Error\r");
      Serial.println("CHECK YOU HAVE CONNECTED TO THE CORRECT SDA AND SCL PIN\r");
      Serial.println("SEE TOP OF SKETCH\r");
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

/*
 * Switch MLX90614 between PWM and SMBUS mode. (and back again and again....) 
 */ 
 
 // This sketch can NOT be run on Artemis V1 or V2  library for Apollo3 from Sparkfun. The Artemis library is 
 // I2C compliant,  but NOT SMBus compliant. SMBus is upper layer specification build on top of I2C. 
  
/* The major issue is that when the SCL or SDA is longer than 50us idle, according to SMBus, the
 * bus is deemed to be free. Any earlier received information needs to be disgarded / reset.
 * 
 * When a read is performed on I2C, first a write of the I2C address + register_to_read is send, then a 
 * second command is send to receive the content of that register. 
 * 
 * On an Artemis V1 library, running clock speed of 200Khz, the delay AFTER completing sending the write and 
 * starting the read is just 50us. On 100Khz it is 88us, thus you read incorrect data. On 400Khz that is 38us 
 * 
 * On Artemis V2 library, mainly due to MBED overhead, the delay at 100Khz is 147Khz, at 200Khz 123us and 
 * with 400Khz 88us. See even at the highest clock speed it is MUCH longer than 50us.
 * 
 * Just to put things into perspective, on an Arduino Uno the delay is around 32us, nearly irrespective 
 * of the clock speed.
 * 
 * When using V1 library it will only work if the clock speed is 200Khz or higher. The MLX90614 is only 
 * supporting 100Khz, so we are out of luck :-(
 */ 
 //            BBBBBBBBBBBBBBUUUUUUUUUUUUUUUUUUUUTTTTTTTTTTTTTTTTTTTTT
 
 // BUT once set (e.g. with an Uno) to PWM it CAN be read with Artemis V1 and V2 !!
 
/* Version 1.0 / November 2021 / paulvha
 * 
 * In order to SWITCH between PWM and SMB :
 * SDA    SDA_PIN
 * SCL    SCL_PIN
 * VCC    PWR_PIN
 * GND    GND
 * 
 * In order to only read in PWM :
 * 
 * SDA    PWM_PIN   // only pin 2 or 3 can handle interrupt on UNO https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
 * SCL    SCL_PIN   // you can select any pin
 * VCC    PWR_PIN or 3V3 
 * GND    GND
 * 
 * Additional remarks
 * 
 * On most boards there are already pull-up resistors so no need to add in that case.
 * 
 * Although it is possible to read BOTH the Ambient and Object temperature in extended PWM mode, the timing 
 * dependency becomes even more critical and also the complexity. We keep it simpler.. you select which
 * temperature you want and it's ranges.
 * 
 * Important might be to look at the clock speed. By default a PWM period is 1Khz (every 1mS). 
 * If you anticipate that this a to heavy burden on your solution, you can reduce that by 
 * increasing the divider: PWM_CLOCK. It does not change accuracy, but the speed with which you get a 
 * next temperature value.
 * 
 * Maybe after some time you do not remember whether the MLX90614 was in SMB or PWM mode. Try the option
 * from the question to read in either mode.
 * 
 *  ================================ Disclaimer ======================================
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  ===================================================================================
 */

// Uno connection
#define SDA_PIN A4 
#define SCL_PIN A5 
#define PWM_PIN 2    // only pin 2 or 3 can handle interrupt on UNO https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
#define PWR_PIN 5    // power pin

// set the temperature to read
// make sure to REMEMBER !!!
#define PWM_OBJ_TEMP true // true = object temperature, false is ambient. 

// Set range. This is important when calculating PWM. Values in celsius.
// make sure to REMEMBER !!!
#define PWM_TEMP_MIN 0   // set the minimum temperature for the range (-20 min)
#define PWM_TEMP_MAX 50  // set the maximum temperature for the range (120 max)

// Set PWM speed range of 00 - 127
// 2 gives 1Khz output frequency
// 4 gives 500Hz 
// 8 gives 250Hz
// Default value to set speed 2Mhz / 2 = 1MHZ clock.
#define PWM_CLOCK 2

// Temperature unit to read (valid SMB mode read only !)
// TEMP_K = in Kelvin,
// TEMP_C = in Celsius,
// TEMP_F = in Fahrenheit
#define SMB_TEMP_UNIT TEMP_C

//*********************************************************************
//*
//*                NO CHANGE NEEDED BEYOND THIS POINT
//*
//*********************************************************************
 
#include <SparkFunMLX90614.h>//Click here to get the library: http://librarymanager/All#Qwiic_IR_Thermometer by SparkFun

IRTherm therm; // Create an IRTherm object to interact with throughout

unsigned long _PWM_HighStartTime;
unsigned long _PWM_LowStartTime;
unsigned long _PWM_Period[2];
unsigned long _PWM_High[2];
bool _PWM_validate[2];
uint8_t _PWM_val = 0;
bool _CalcDutyCycle = false;
bool ReadAsPWM;

// store the read range
float _Tmax = PWM_TEMP_MIN;
float _Tmin = PWM_TEMP_MAX;

// prototype
float getPWMTemperature(bool fahrenheit = false);

void setup() 
{
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println(F("Set for SMB or PWM for MLX60914"));

  // enable power
  if(PWR_PIN != 0) {
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite (PWR_PIN, HIGH);

    // allow delay to start
    delay(500);
  }
 
  //disable wire for now
  Wire.end();

  char c = 'n';
  while (c == 'n' || c == 'N') {
    
    Serial.println(F("Want to change from SMB to PWM  (Y/N)?\r"));
    c = ReadInput();
    if (c == 'y' || c == 'Y') {
      SMB_to_PWM();
      ReadAsPWM = true;
    }

    if (c == 'n' || c == 'N') {
      Serial.println(F("Want to change from PWM to SMB  (Y/N)?\r"));
      c = ReadInput();
      
      if (c == 'y' || c == 'Y') {
        PWM_to_SMB();
        ReadAsPWM = false;
      }
    }
    
    if (c == 'n' || c == 'N') {  
      Serial.println(F("Want to start reading in PWM (Y/N)?\r"));
      c = ReadInput();
      
      if (c == 'y' || c == 'Y') {
      ReadAsPWM = true;
      }
    }
    
    if (c == 'n' || c == 'N') {
      Serial.println(F("Want to start reading in SMB (Y/N)?\r"));
      c = ReadInput();
      
      if (c == 'y' || c == 'Y') {
      ReadAsPWM = false;
      }
    }
  }

  // intitialise the library to read
  Init();
}

void loop() {
  
  if(ReadAsPWM) { // get as PWM
    
    if(PWM_OBJ_TEMP) Serial.print(F("PWM Object temperature: "));
    else Serial.print(F("PWM Ambient temperature: "));
    Serial.print(getPWMTemperature());
    Serial.print(" C\t");
    Serial.print(getPWMTemperature(true));
    Serial.println(" F\r");
  }
  else {         // get as SMB
  
    // Call therm.read() to read object and ambient temperatures from the sensor.
    if (therm.read()) // On success, read() will return 1, on fail 0.
    {
      // Use the object() and ambient() functions to grab the object and ambient
      // temperatures.
      // They'll be floats, calculated out to the unit you set with setUnit().
      Serial.print("Object: " + String(therm.object(), 2));
      if (SMB_TEMP_UNIT == TEMP_C)   Serial.println(" C\r");
      else if (SMB_TEMP_UNIT == TEMP_F) Serial.println(" F\r");
      else Serial.println(" K\r");
      
      Serial.print("Ambient: " + String(therm.ambient(), 2));
      if (SMB_TEMP_UNIT == TEMP_C)   Serial.println(" C\r");
      else if (SMB_TEMP_UNIT == TEMP_F)   Serial.println(" F\r");
      else    Serial.println(" K\r");
      
      Serial.println();
    }
  }
    
  delay(1000);
}

/* enable reading in PWM mode */
void InitPWM()
{
  Wire.end();

  // set SCL high during read
  pinMode(SCL_PIN, OUTPUT);
  digitalWrite(SCL_PIN, HIGH); 
  
  // set PWM pin for input
  pinMode(PWM_PIN,INPUT);

  // init received values
  for(uint8_t i =0; i < 2;i++) {
    _PWM_Period[i] = _PWM_High[i] = 0;
    _PWM_validate[i] = false;
  }

  _PWM_HighStartTime = _PWM_LowStartTime = 0;
  
  // set function to call.
  attachInterrupt(digitalPinToInterrupt(PWM_PIN), PWM_isr,  CHANGE);
}

/* called when level changes on the PWM line */
void PWM_isr()
{
  // Capture current time
  unsigned long bitTime = micros();

  // if high
  if (digitalRead(PWM_PIN)) {
  
    if (_PWM_HighStartTime == 0) {
  
      _PWM_HighStartTime = bitTime;
      _PWM_LowStartTime = 0;
  
    }
    else if (_PWM_HighStartTime != 0 && _PWM_LowStartTime != 0) {
  
      // do not move forward when calculating duty cycle
      if ( !_CalcDutyCycle ) {
  
        // check for a time overrun / turn around after many days power-on
        if (bitTime > _PWM_HighStartTime)
        {
          _PWM_Period[_PWM_val] = bitTime - _PWM_HighStartTime;
          _PWM_High[_PWM_val] = _PWM_LowStartTime - _PWM_HighStartTime;

          // indicate as complete
          _PWM_validate[_PWM_val] = true;

          // keep 2 different values
          _PWM_val++;
          if(_PWM_val > 1) _PWM_val = 0;
        }
      }
  
      // re-init
      _PWM_HighStartTime = 0;
      _PWM_LowStartTime = 0;
      _PWM_validate[_PWM_val] = false;
    }
  }
  else { // low and thus of T2
    if (_PWM_HighStartTime != 0) {
      _PWM_LowStartTime = bitTime;
    }
  } 
}

/* initialise for reading */
void Init()
{
  if (ReadAsPWM) {
    Serial.print(F("For PWM reading : make sure the SDA-line of the MLX is connected to pin "));
    Serial.println(PWM_PIN);
   
    InitPWM();
  }
  else {
    Serial.println(F("Make sure the MLX is connected to SDA/SCL.\r"));
    StartIIR();
    therm.setUnit(SMB_TEMP_UNIT);
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
  
  // return input first character
  return(Serial.read());
}

/* set MLX permanently in SMB mode */
void PWM_to_SMB()
{
  if( SCL_PIN == 0 || PWR_PIN == 0){
    Serial.println(F("Can not perform switch, Missing Power and/or SCL pin\r"));
    while(1);
  }

  Serial.print(F("** Make sure the MLX SDA line is connected to the SELECTED SDA pin "));
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
    Serial.println(F("Can not perform switch, Missing Power and/or SCL pin\r"));
    while(1);
  }
  
  SMB_OR_PWM(false);
}

/* enable I2C/SMBUS interaction with MLX90614 */
void StartIIR()
{
  Wire.begin();
  delay(500);
  
  if (therm.begin() == false){ // Initialize the MLX90614
    Serial.println(F("MLX90614 thermometer did not acknowledge! Freezing!"));
    while(1);
  }
  Serial.println(F("MLX90614 thermometer acknowledged."));
}

/**
 * set minimum or maximum temperature for PWM
 * @param min
 *  true  : set minimum temperature  
 *  false : set maximum temperature
 *  
 *  Reading and setting the range is incorrect in the current version of the Sparkfun library
 */
void SetLimit(float t, bool min)
{
  uint16_t a;

  // set for Object temperature
  if(PWM_OBJ_TEMP){
  
    a = (t + 273.15) * 100;
    
    if (min) {
      if( !therm.writeEEPROM(MLX90614_REGISTER_TOMIN, a))
      {
        Serial.println(F("Error during writing MLX90614_REGISTER_TOMIN. Freeze\r"));
        while(1); 
      }
    }
    else {
      if( !therm.writeEEPROM(MLX90614_REGISTER_TOMAX, a))
      {
        Serial.println(F("Error during writing MLX90614_REGISTER_TOMAX. Freeze\r"));
        while(1); 
      }
    }
  }
  else {

    // read current ambient temperature values
    if( !therm.I2CReadWord(MLX90614_REGISTER_TARANGE, &a))
    {
      Serial.println(F("Error during reading MLX90614_REGISTER_TARANGE. Freeze\r"));
      while(1); 
    }
 
    uint16_t b = (uint16_t) ((t + 38.2) * 100/64); 

    if(min) {
      a &= 0xff00;          // clear LSB
      a |= (b & 0xff);  
    }
    else {
      a &= 0xff;            // clear MSB
      a |= (b & 0xff) << 8;
    }

    if( !therm.writeEEPROM(MLX90614_REGISTER_TARANGE, a))
    {
      Serial.println(F("Error during writing MLX90614_REGISTER_TARANGE. Freeze\r"));
      while(1); 
    }   
  }
}

/**
 * Read minimum or maximum temperature for PWM
 * @param min
 *  true : read minimum temperature  
 *  false : read maximum temperature
 *  
 * Reading and setting the range is incorrect in the current version of the Sparkfun library
 */
float ReadLimit(bool min)
{
  uint16_t a;
  
  // read object temperature value
  if(PWM_OBJ_TEMP){
    
    if (min) {
      if( !therm.I2CReadWord(MLX90614_REGISTER_TOMIN, &a))
      {
        Serial.println(F("Error during reading MLX90614_REGISTER_TOMIN. Freeze\r"));
        while(1); 
      }
    }
    else {
  
      if( !therm.I2CReadWord(MLX90614_REGISTER_TOMAX, &a))
      {
        Serial.println(F("Error during reading MLX90614_REGISTER_TOMAX. Freeze\r"));
        while(1); 
      }
    }
      
    return((float) (a / 100) - 273.15);
  }
  
  // read ambient min & max temperature value
  if( !therm.I2CReadWord(MLX90614_REGISTER_TARANGE, &a))
  {
    Serial.println(F("Error during reading MLX90614_REGISTER_TARANGE. Freeze\r"));
    while(1); 
  }

  if (min) a &= 0xff;          // use LSB  
  else  a = (a & 0xff00) >> 8; // use MSB
  
  return((float) (a * 64 / 100) - 38.2);
}

/* set temperature source: Ambient or Object */
void SelectPWMTemp()
{
   uint16_t val;

   // read current config
  if (! therm.I2CReadWord(MLX90614_REGISTER_CONFIG, &val)){
    Serial.println(F("Error during MLX90614_REGISTER_CONFIG. Freeze\r"));
    while(1); 
  }

  if(PWM_OBJ_TEMP) val |= 0x30;   // bit 4 and 5 set to 1 is IR1
  else val &= 0xffcf;             // bit 4 and 5 set to 0 is TA  

  if( !therm.writeEEPROM(MLX90614_REGISTER_CONFIG, val))
  {
    Serial.println(F("Error during writing MLX90614_REGISTER_CONFIG. Freeze\r"));
    while(1); 
  }
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
  
  // enable I2C/SMBUS
  StartIIR();
  
  // range only valid for PWM
  if (!act) {
    
    // set range
    SetLimit(PWM_TEMP_MIN, true);
    SetLimit(PWM_TEMP_MAX, false);
  
    // read min
    Serial.print(F("min. Temp Range "));
    _Tmin = ReadLimit(true);
    Serial.print(_Tmin);
    Serial.println(" C");
  
    // read max
    Serial.print(F("max. Temp Range "));
    _Tmax = ReadLimit(false);
    Serial.print(_Tmax);
    Serial.println(" C");
  }
  
  // read current PWMconfig
  if (! therm.I2CReadWord(MLX90614_REGISTER_PWMCTRL, &val)){
    Serial.println(F("Error during reading PWMCTRL. Freeze\r"));
    while(1); 
  }
  // debug only
//Serial.print("val before ");
//Serial.println(val,HEX);

  // I got 0x201 as a valid default number on my MLX90614, 
  // This is just an extra check to make sure have some reasonable number before updating and writing back.
  // val = 0x201;  // uncomment line to reset to my default
  if (val == 0x0 || val == 0xffff) {
    Serial.print(F("Something went wrong in reading.\nGot current PWMCTRL register as 0x"));
    Serial.println(val, HEX);
    Serial.println(F("Expected something like 0x201. Check wiring and read in function SMB_OR_PWM\r"));
    while(1);
  }
  // set mode
  if (act) {
    val &= 0xfffd;               // set SMB (bit 1 = 0)       
  }
  else {
    val |= 0x0002;               // set PWM (Bit = 1)
    val &= 0xfffb;               // set opendrain instead of push pull (Bit 2 = 0)
    val &= 0xfff7;               // set PWM instead of ThermoRelay (Bit 3 = 0)
    val &= 0x00ff;               // remove current divider
    val |= PWM_CLOCK<<8;         // set new speed divider

   // select temperature source for single PWM
   SelectPWMTemp();
  }
  
  // debug only
//Serial.print(F("val after update "));
//Serial.println(val,HEX);

  // update PWM config
  if( !therm.writeEEPROM(MLX90614_REGISTER_PWMCTRL, val))
  {
    Serial.println(F("Error during writing MLX90614_REGISTER_PWMCTRL. Freeze\r"));
    while(1); 
  }

  delay(1000);
  
  Wire.end();

  // now reboot to enable new situation
  reboot();
}

/*******************************************************************
    Function Name: getPWMTemperature
    Description:  Calculate and return the temperature from PWM
    Parameters:
    bool: true for Fahrenheit scale, false (or unspec) for Celsius scale
    Return: Measured temperature or zero if invalid
 ******************************************************************/
float getPWMTemperature(bool fahrenheit)
{
  float pwm_period, pwm_high, duty;
  uint8_t off = _PWM_val ? 0 : 1;

  // if no valid data yet
  if (! _PWM_validate[off]) return (0);

  // prevent data overwrite
  _CalcDutyCycle = true;

  pwm_period = _PWM_Period[off];
  pwm_high = _PWM_High[off];

  // re-enable data capture
  _CalcDutyCycle = false;
/*
// debug only
Serial.print("\nhigh ");
Serial.print(pwm_high);
Serial.print(" pwm_period ");
Serial.print(pwm_period);

Serial.print(" _Tmax ");
Serial.print(_Tmax);

Serial.print(" _Tmin ");
Serial.println(_Tmin);
*/
  duty = (pwm_high - (pwm_period / 8)) / pwm_period;

  float t = (2 * duty) * (_Tmax - _Tmin)  + _Tmin;

  return fahrenheit ? (t * 1.8) + 32.0 : t;
}

/* reboot the MLX90614  */
void reboot() {
  // toggle the power
  digitalWrite (PWR_PIN, LOW);
  delay(3000);
  digitalWrite (PWR_PIN, HIGH);

  // allow delay to start
  delay(1000);
}

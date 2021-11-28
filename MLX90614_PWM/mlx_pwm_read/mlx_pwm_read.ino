/*
 * This sketch will read an MLX90614 once it has been set to PWM mode
 * It can be run on Artemis Sparkfun Library V1 and V2
 */ 
 // Make sure to set :
/* The right board
 * Pins to connect the SCL and SDA line from the MLX90614
 * set the PWM_OBJ_TEMP: to indicate what the source is (object or ambient)
 * Set the PWM_TEMP_MIN and PWM_TEMP_MAX 
 * 
 * Connect In order to read in PWM :
 * 
 * SDA    PWM_PIN
 * SCL    SCL_PIN
 * VCC    PWR_PIN or 3V3
 * GND    GND
 * 
 * Version 1.0 / November 2021 / pauvha
 * 
 * Additional remarks
 * 
 * Although you may not know whether it is the Ambient or Object temperature, when you hold 
 * your hand on a short distance in front of the MLX90614 and the temperature reading changes 
 * immediately.. it is object temperature.
 * 
 * You can connect an MLX90614 directly to the 3V3 or use one of the pins (PWR_PIN). The later
 * can help power consumption if you only want to measure once in while.
 * 
 * If you only want to read the temperature on a certain moment and to reduce the impact of 
 * constant interrupt caused by the PWM signal, we use in this example PWM_begin() and PWM_end()
 * before and after each reading.
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
 
// indicate this sketch will be on an Apollo library
#define APOLLO3_LIB

#ifdef APOLLO3_LIB

#define PWM_PIN 15 // PWM reading to connect SDA line from MLX90614, you can select any pin
#define SCL_PIN 26 // You can select any pin.
#define PWR_PIN 9  // power pin (set to 0 -zero- if direct connected to 3v3)

#else

// Example for Uno
#define SCL_PIN 7  // you can select any pin.
#define PWM_PIN 2  // only pin 2 or 3 can handle interrupt on UNO https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
#define PWR_PIN 5  // power pin (set to 0 -zero- if direct connected to 3v3)

#endif

// set the temperature thtat is being read. This can NOT be detected from the datastream.
// you have to remember from when you set the MLX90614 in PWM mode
#define PWM_OBJ_TEMP true // true = object temperature, false is ambient. 

// Set range. This is important when calculating PWM. Values in celsius
// This can NOT be detected from the datastream
// you have to remember from when you set the MLX90614 in PWM mode
#define PWM_TEMP_MIN 0   // set the minimum temperature for the range (-20 min)
#define PWM_TEMP_MAX 50  // set the maximum temperature for the range (120 max)

//*********************************************************************
//*
//*                NO CHANGE NEEDED BEYOND THIS POINT
//*
//*********************************************************************

unsigned long _PWM_HighStartTime;
unsigned long _PWM_LowStartTime;
unsigned long _PWM_Period[2];
unsigned long _PWM_High[2];
bool _PWM_validate[2];
uint8_t _PWM_val = 0;
bool _CalcDutyCycle = false;

// prototype
float getPWMTemperature(bool fahrenheit = false);

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println(F("Reading MLX60914 in single PWM"));

  Serial.print(F("For PWM reading make sure the \nSDA-line of the MLX is connected to pin "));
  Serial.println(PWM_PIN);
  Serial.print(F("SCL-line of the MLX is connected to pin "));
  Serial.println(SCL_PIN);
  
  PWM_begin();
}

void loop() {

  // this is just to show that you can start and turn-off after reading
  // else you can do on-time PWM_begin() in setup() and remove PWM_end()
  PWM_begin();
  
  // wait for first reading
  while (!getPWMTemperature());
  
  if(PWM_OBJ_TEMP) Serial.print(F("PWM Object temperature: "));
  else Serial.print(F("PWM Ambient temperature: "));
  Serial.print(getPWMTemperature());
  Serial.print(" C\t");
  Serial.print(getPWMTemperature(true));
  Serial.println(" F\r");
   
  PWM_end();

  // delay totals 3.5 seconds (0.5 second in PWM_begin())
  delay(3000);
}

/* enable reading in PWM mode */
void PWM_begin()
{
  // enable power
  if(PWR_PIN != 0) {
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite (PWR_PIN, HIGH);
  }  
  
  // set SCL high during read
  pinMode(SCL_PIN, OUTPUT);
  digitalWrite(SCL_PIN, HIGH); 
  
  // set PWM pin for input
  pinMode(PWM_PIN,INPUT);

  // init received values
  for(uint8_t i = 0; i < 2; i++) {
    _PWM_Period[i] = _PWM_High[i] = 0;
    _PWM_validate[i] = false;
  }

  _PWM_HighStartTime = _PWM_LowStartTime = 0;
  
  // set function to call.
  attachInterrupt(digitalPinToInterrupt(PWM_PIN), PWM_isr,  CHANGE);

  // give time to settle
  delay(500);
}

/* end PWM reading */
void PWM_end()
{
  digitalWrite(SCL_PIN, LOW); 

  detachInterrupt(PWM_PIN);

  // disable power
  if(PWR_PIN != 0) digitalWrite (PWR_PIN, LOW);

  _PWM_validate[0] = _PWM_validate[1] = false;
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

/*******************************************************************
    Function Name: getPWMTemperature
    Description: Calculate and return the temperature from PWM
    Parameters:
    bool: true for Fahrenheit scale, false for Celsius scale
    Return: Measured temperature or zero if invalid
 ******************************************************************/
float getPWMTemperature(bool fahrenheit)
{
  float pwm_period, pwm_high, duty;
  uint8_t off = _PWM_val ? 0 : 1;

  // if no valid data yet
  if(! _PWM_validate[off]) return (0);

  // prevent data overwrite
  _CalcDutyCycle = true;

  pwm_period = _PWM_Period[off];
  pwm_high = _PWM_High[off];

  // re-enable data capture
  _CalcDutyCycle = false;

  duty = (pwm_high - (pwm_period / 8)) / pwm_period;

  float t = (2 * duty) * (PWM_TEMP_MAX - PWM_TEMP_MIN)  + PWM_TEMP_MIN;

  return fahrenheit ? (t * 1.8) + 32.0 : t;
}

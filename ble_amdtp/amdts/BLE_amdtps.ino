/*
  By: Owen Lyke
  SparkFun Electronics
  Date: Aug 27, 2019
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.
  ***********************************************************************************
  paulvha/ February 2020 / version 1.0
   *initial version
   

  paulvha/ February 2020 / version 1.1
  * updated timer handling

  ************************************************************************************

  Based on the original sample (see below) this example will allow data exchange between 
  Linux system running amdtp-service on Bluez. The client will take the initiative to 
  sent a request/command which this server will handle and respond back to the client.

  Optional support for BME280 connected to 'qwuic' interface. You have to UNCOMMENT
  the line 76. 
  The has been tested with an Adafruit BME280. Parts of the code below are coming from 
  Sparkfun BME280 library. The expected I2C address is 0x77.

    BMW280      QWUIC
    GND         GND
    VCC         3V3
    SCK         SCL
    SDI         SDA

  There are different DEBUG levels that can be set in ble_debug.h
  
  BLE_Debug : will show all the data and program flow
  BLE_SHOW_DATA : will only show the data received and sent

  Make sure to installed the amdtc-client on linux system ( e.g. Raspberry Pi)
   
  *************** original heading ******************************
  This example demonstrates basic BLE server (peripheral) functionality for the Apollo3 boards.
  How to use this example:
    - Install the nRF Connect app on your mobile device (must support BLE bluetooth)
    - Make sure you select the correct board definition from Tools-->Board
      (it is used to determine which pin controls the LED)
    - Compile and upload the example to your board with the Arduino "Upload" button
    - In the nRF Connect app look for the device in the "scan" tab.
        (By default it is called "Artemis BLE" but you can change that below)
    - Once the device is found click "connect"
    - The GATT server will load with 5 services:
      - Generic Access
      - Generic Attribute
      - Link Loss
      - Immediate Alert
      - Tx Power
    - For this example we've hooked into the 'Immediate Alert' service.
        You can click on that pane and it will expand to show an "upload"  button.
        Use the upload button to write one of three values (0x00, 0x01, or 0x02)
    - When you send '0x00' (aka 'No alert') the LED will be set to off
    - When you send either '0x01' or '0x02' the LED will be set to on
*/

#include "BLE_amdtp.h"
#include "apollo3.h"                  // needed for battery load resistor

// Up to 29 characters
#define BLE_PERIPHERAL_NAME "Artemis AMDTP BLE" 

// maximum length of reply / data message
#define MAXREPLY 100

// maximum retry to read ADC level to improve increased quality
#define ADC_RETRY 3

/////////////////////////////////////////////////////////////////////////////
// include BME280 as option, REMOVE the comments to include
////////////////////////////////////////////////////////////////////////////
//#define INCLUDE_BME280 1

#ifdef INCLUDE_BME280

  // sent Altitude in meters (true) or feet (false)
  bool ALTM = true;

  // sent temperature in Celsius (true) or Fahrenheit (false)
  bool TEMPC = true;
/////////////////////////////////////////////////////////////////////
//
//  NO NEED FOR CHANGES AFTER THIS POINT
//
////////////////////////////////////////////////////////////////////
  
  #include <Wire.h>
  #include "SparkFunBME280.h"
  BME280 mySensor;

  /* needed for conversion float IEE754 */
  typedef union {
      byte array[4];
      float value;
  } ByteToFloat;

  // indicator BME280 detected 
  bool BmeDetected = false;
  
#endif // INCLUDE_BME280

// buffer to reply to client
uint8_t  val[MAXREPLY];
uint8_t  *val_data = &val[2];   // start of optional data area
uint8_t  *val_len  = &val[1];   // store length of optional data

// for testdata
uint16_t TestCounter = 0;

// for chat
bool opt_chat = false;

/***********************************************************************/
void setup() {

#if defined BLE_SHOW_DATA
    SERIAL_PORT.begin(115200);
    delay(1000);
    SERIAL_PORT.printf("Apollo3 BLE AMDTP protocol. Compiled: %s\n", __TIME__);
#endif

  pinMode(LED_BUILTIN, OUTPUT);
  set_led_low();

#ifdef INCLUDE_BME280
  Wire.begin();

  if (mySensor.beginI2C() == false) //Begin communication over I2C
  {
#if defined BLE_SHOW_DATA
    Serial.println("The BME280 did not respond. Please check wiring.");
#endif
  }
  else
    BmeDetected = true;
#endif //INCLUDE_BME280

  //
  // Configure the peripheral's advertised name:
  setAdvName(BLE_PERIPHERAL_NAME);
  
  //
  // Boot the radio.
  //
  HciDrvRadioBoot(0);

  //
  // Initialize the main ExactLE stack.
  //
  exactle_stack_init();

  //
  // Start the "Amdtp" profile.
  //
  AmdtpStart();
}

void loop() {

  if (opt_chat) chat();
  
  trigger_timers();
}


void trigger_timers()
{
  //
  // Calculate the elapsed time from our free-running timer, and update
  // the software timers in the WSF scheduler.
  //
  update_scheduler_timers();
  wsfOsDispatcher();            // start any handlers if event is pending on them
}

/* 
 * This routine is called when data / cmd has been received from the client
 * 
 * The case-values ( like AMDTP_CMD_HELLO) needs to stay in sync with
 * the eAmdtpPktcmd_t definitions in BLE_amdtp.h
 */

void UserRequestReceived(uint8_t * buf, uint16_t len)
{
    #if defined BLE_SHOW_DATA
      SERIAL_PORT.printf("received command with length %d : ", len);
      for (uint16_t i = 0 ; i< len ; i++) SERIAL_PORT.printf("0x%X ", buf[i]);
    #endif
    
    uint16_t value;
    val[0] = buf[0];    // echo requested command
    *val_len = 0;       // set for zero optional data
    val_data = &val[2]; // set start of optional data
    
    trigger_timers();
    
    switch (buf[0]) {

      case AMDTP_CMD_START_TEST_DATA:
        #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("Send test data");
        #endif
        
        send_test_data();
        break;

      case AMDTP_CMD_STOP_TEST_DATA:
        #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("Stop test data");
        #endif
        
        rest_test_data();
        break;
        
      case AMDTP_CMD_HELLO:

        #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("Received HELLO / wakeup from client");
        #endif
        break;

      case AMDTP_CMD_TURN_LED_ON:

        #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("Turn Led on");
        #endif

        set_led_high();
        break;

      case AMDTP_CMD_TURN_LED_OFF:

        #if defined BLE_SHOW_DATA
           SERIAL_PORT.println("Turn Led off");
        #endif

        set_led_low();
        break;

      case AMDTP_CMD_REQ_BATTERY_LEVEL:

        #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("Read internal battery level");
        #endif

        // read battery percentage
        *val_data = read_battery_perc();
        *val_len = 1;
        break;
        
      case AMDTP_CMD_REQ_BATTERYLOAD_ON:
        
        #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("set Batteryload");
        #endif
        
        MCUCTRL->ADCBATTLOAD_b.BATTLOAD = 1;              
        break;
        
      case AMDTP_CMD_REQ_BATTERYLOAD_OFF:
        
        #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("remove Batteryload");
        #endif  
        
        MCUCTRL->ADCBATTLOAD_b.BATTLOAD = 0;      
        break;
        
      case AMDTP_CMD_REQ_INTERNAL_TEMP_FRH:

        #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("Read internal temperature in Fahrenheit");
        #endif
        
        // read internal temperature
        value = read_Internal_temp(1);
        *val_data++ = value >> 8;    // MSB
        *val_data = value & 0xff;  // LSB

        *val_len = 2;
        break;

      case AMDTP_CMD_REQ_INTERNAL_TEMP_CEL:
        
        #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("Read internal temperature in Celsius");
        #endif

        // read internal temp
        value = read_Internal_temp(2);
        *val_data++ = value >> 8;    // MSB
        *val_data = value & 0xff;  // LSB

        *val_len = 2;
        break;

      case AMDTP_CMD_ADC:
         
        #if defined BLE_SHOW_DATA
            SERIAL_PORT.print("Read ADC channel :");
            SERIAL_PORT.println(buf[1]);
        #endif

        value = read_adc(buf[1]);
        *val_data++ = buf[1];       // ADC channel read
        *val_data++ = value >> 8;   // MSB
        *val_data = value & 0xff;   // LSB   

        *val_len = 3;
        break;

      case AMDTP_CMD_CHAT:

        #if defined BLE_SHOW_DATA
            SERIAL_PORT.print("Chat");
        #endif
        
        // if not started already, do it now
        SERIAL_PORT.begin(115200);
        
        // display received message
        Serial.print("\n<<<<< ");
        Serial.println((char *) &buf[1]);
        
        // if received BYE stop chat and echo back
        // else chat a new line.
        if (len == 5) {
          if (strcmp((char *) &buf[1], "BYE") == 0) {
            strcpy((char *) &val_data,"BYE");
            *val_len = 3;
            Serial.println("\nChat cancelled");
            break;
          }
        }

        Serial.print(">>>>> ");
        opt_chat = true;        // will be called from loop to enable bluetooth keep-alive
        return;

      case AMDTP_CMD_READ_PIN:

        #if defined BLE_SHOW_DATA
            SERIAL_PORT.print("Digital read pin: ");
            SERIAL_PORT.println(buf[1]);
        #endif
        
        *val_data++ = buf[1];     // pin read
        *val_data++ = digitalRead(buf[1]);

        *val_len = 2;
        break;
      
      case AMDTP_CMD_PIN_HIGH:
         
        #if defined BLE_SHOW_DATA
            SERIAL_PORT.print("Set HIGH digital pin: ");
            SERIAL_PORT.println(buf[1]);
        #endif

        *val_data++ = buf[1];     // pin 
        *val_len = 1;
        
        pinMode(buf[1], OUTPUT);
        digitalWrite(buf[1], HIGH);
        break;

      case AMDTP_CMD_PIN_LOW:
         
        #if defined BLE_SHOW_DATA
            SERIAL_PORT.print("Set LOW digital pin: ");
            SERIAL_PORT.println(buf[1]);
        #endif

        *val_data++ = buf[1];     // pin 
        *val_len = 1;
        
        pinMode(buf[1], OUTPUT);
        digitalWrite(buf[1], LOW);
        break;

      case AMDTP_CMD_BME280:
     
#ifdef INCLUDE_BME280

        // if detected 
        if (BmeDetected) {
        
          #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("Read BME280");
          #endif
        
          Read_BME280();
        }
        else {  //length is zero to be detected by client
          
          #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("Read BME280: NOT DETECTED");
          #endif         
        }
#else
        #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("Read BME280: NOT ENABLED");
        #endif      
 
#endif //INCLUDE_BME280
        break;
        
     case AMDTP_CMD_CUSTOM1:      // repeat for other options
         
        #if defined BLE_SHOW_DATA
            SERIAL_PORT.println("AMDTP_CMD_CUSTOM1");
        #endif

        // Do something here....
        break;
        
     default:
        val[0] = 0;             // indicate no defined action for this request
        break;
    }

    SendReplyClient();
}

/**
 * perform a simple chat
 * 
 * Will capture a line of data and terminate with 0x0 on CR of NL.
 * Typing:  BYE will cause end of chat
 * 
 */
void chat()
{
  if (Serial.available()) {
 
    *val_data = Serial.read();
    *val_len += 1;
    
    if (*val_data == 0x0d || *val_data == 0x0a){
        *val_data = 0x0;
        Serial.println((char *) &val[2]); // display on local screen
        SendReplyClient();
        opt_chat = false;
    }
    else
      val_data++;
  }
}

/*
 * This was originally in amdtp_main.c but it did not work as we need to
 * keep the wsf-timing alive, so moved to user level.
 */
void send_test_data()
{

 // wait in between sending
 //if(TestCounter > 0) delay(2000);

 *val_data++ = 'T';
 *val_data++ = 'e';
 *val_data++ = 's';
 *val_data++ = 't';
 *val_data++ = ':';
 *val_data++ = 0x0;

 *val_data++ = TestCounter >> 8; // MSB
 *val_data = TestCounter;        // LSB

 *val_len = 8;
 
 TestCounter++;
}

void rest_test_data()
{
  TestCounter = 0;
}


#ifdef INCLUDE_BME280
/**
 * @brief : translate float IEEE754 to val[x]
 * @param value : float value to add
 * @param p : offset in buffer
 *
 * return : float number
 */
void float_to_byte(float value, uint8_t *p)
{
  ByteToFloat conv;
  conv.value = value;
  for (byte i = 0; i < 4; i++) *p++ = conv.array[3-i];
}

/**
 * Read the BME280 values into the buffer
 */
void Read_BME280()
{
  float ret;
  uint8_t symbol;

  //*************** TEMPERATURE **************
  if (TEMPC) {
    ret = mySensor.readTempC();
    symbol = 'C';
  }
  else {
    ret = mySensor.readTempF();
    symbol = 'F';
  }
  *val_data++ = symbol;
  
  float_to_byte(ret, val_data);
  val_data += 4;
  *val_len += 5;
  
#if defined BLE_SHOW_DATA     
  Serial.print(" Temp: ");
  Serial.print(ret,2);
  Serial.print(" ");
  Serial.println((char) symbol);
#endif 

  //****************** HUMIDITY *****************
  ret = mySensor.readFloatHumidity();
  float_to_byte(ret, val_data);
  val_data += 4;
  *val_len += 4;
#if defined BLE_SHOW_DATA     
  Serial.print("Humidity: ");
  Serial.print(ret);
#endif

  //****************** PRESSURE *****************
  ret = mySensor.readFloatPressure();
  float_to_byte(ret, val_data);
  val_data += 4;
  *val_len += 4;
#if defined BLE_SHOW_DATA     
  Serial.print(" Pressure: ");
  Serial.print(ret);
#endif

  //****************** ALTITUDE *****************
  if (ALTM) {
    ret = mySensor.readFloatAltitudeMeters();
    symbol = 'M';
  }
  else {
    ret = mySensor.readFloatAltitudeFeet();
    symbol = 'F';
  }
  *val_data++ = symbol;
  
  float_to_byte(ret, val_data);
  val_data += 4;
  *val_len += 5;
#if defined BLE_SHOW_DATA     
  Serial.print(" Alt: ");
  Serial.print(ret);
  Serial.print(" ");
  Serial.println((char) symbol);
#endif

}
#endif //INCLUDE_BME280

/**
 * read internal temperature value
 * @param v =
 *  1 = Fahrenheit
 *  2 = Celsius
 */

uint16_t read_Internal_temp(uint8_t v)
{
  uint16_t tempadc = read_adc(2);
  float fADCTempVolts;
  float fTemp, fCalibration_temp, fCalibration_voltage, fCalibration_offset;

  if (tempadc == 0) return(0);
  //
  // Convert and scale the temperature.
  // Temperatures are in Fahrenheit range -40 to 225 degrees.
  // Voltage range is 0.825V to 1.283V
  // First get the ADC voltage corresponding to temperature.
  //
  fADCTempVolts = (float) tempadc * 2.0 / 1024.0f;

  //
  // Get temperature from trimmed values & convert to degrees K.
  //
  // Trim can be really measured/obtained for this chip or default values.
  //
  // It seems that default values is what normally is the case and then the results might be off.
  // According to the source: Since the device has not been calibrated on the tester, we'll load
  // default calibration values. These default values should result
  // in worst-case temperature measurements of +-6 degress C. (SIX degrees Celsius!!!)
  //
  // I have 2 edge modules and had them run for an hour in a loop before taking these values:
  // Measured with external temperature meter next tot the edge board it is around 20.5 C.
  // one module gives 16.5 C  - 17.5 C
  // the other gives  19.5 C - 20 C
  //

  fCalibration_temp = AM_HAL_ADC_CALIB_TEMP_DEFAULT;
  fCalibration_voltage = AM_HAL_ADC_CALIB_AMBIENT_DEFAULT;
  fCalibration_offset  = AM_HAL_ADC_CALIB_ADC_OFFSET_DEFAULT;

  //
  // Compute the temperature.
  //
  fTemp  = fCalibration_temp;                             // k
  fTemp /= (fCalibration_voltage - fCalibration_offset);  // k / v
  fTemp *= (fADCTempVolts - fCalibration_offset);         // k /v * v = k

  if (v == 1)   // if Fahrenheit
    return( (int16_t) 10 *  (((fTemp - 273.15f) * 180.0f / 100.0f) + 32.0f));

  else          // celsius
    return ((int16_t) ((fTemp - 273.15f) * 10));
}

/**
 * read the Battery ADC level provide the percentage
 */

uint8_t read_battery_perc()
{
    uint16_t battadc = read_adc(1);

  /* The power supply / battery level is provided as devided by 3, so the real voltage * 3.
   * the reference voltage is set as 2.0 internal.  (ap3_analog.cpp)
   * ADC is set to 10 bit to keep compatible with Arduino (ap3_analog.cpp)
   *
   * Assume battery voltage is 3V, DIV3 will provide 1V
   * reference voltage is (2V / 1024) : ADC reading will be around 512
   *
   * SO an ADC reading is 512 * 3 = 1536 * (2 / 1024) ~ 3V
   */

  float val = (float) (battadc * 3.0f * 2.0 / 1024.0f);

  /* turn into percentage + 0.4f in case it is XX.6%*/
  uint8_t perc =  (uint8_t) (val*100 / 3.3f + 0.5f);
  
  return (perc);
}

/**
 * read the ADC level on the Artemis
 * @param R =
 *  1 : read internal ADC for battery level
 *  2 : read internal ADC for internal temperature
 *      else it is the ADC pin to read
 *
 */
uint16_t read_adc(uint8_t R) {

  int i, j, k, rr, pin;
  uint16_t val = 0;

  if (R == 1) pin = ADC_INTERNAL_VCC_DIV3;  // battery get adc voltage divided by 3
  else if (R == 2) pin = ADC_INTERNAL_TEMP; // get internal temperature value
  else pin = R;

  // OftenanalogRead fails (read zero) or provides previous data on Apollo hence
  // a couple of ways to handle :
  // 1 read first a dummy to flush old reading
  // 2 Retry a couple of times and sent average
  // 3 ignore zero reading for 10 times max.
  // update : the root cause is known but not fixed in version 1.0.29 (maybe later versions)

  // read dummy to flush
  analogRead(pin);

  j = ADC_RETRY;
  k = 0;          // dead loop control

  for(i=0; i < j ; i++) {
    rr = analogRead(pin);

    if (rr == 0) {                  // if zero reading
       if (k++ < 10) j++;           // retry once again BUT no more than 10 times
       else return 0;               // dead loop control
    }

    else val = val + rr;
     //SERIAL_PORT.print("rr = ");   // uncommment for debug only
     //SERIAL_PORT.println(rr);
  }
  val = val / (i - k);              // the number of sample might have been extended because of ZERO reading
  return(val & 0x3ff);              // 10 bits is standard, 1024 -> 0x400
}

/**
 *  sent a data message to the client
 *  
 * Format received data :
 * buf [0] = request that was sent
 * buf [1] = length of additional data
 * buf [2] ..... additional data
 */
void SendReplyClient()
{
  if (AmdtpSendData(val, *val_len + 2) == false) {

    #if defined BLE_Debug
      SERIAL_PORT.printf("Failed to sent data for client\n");
    #endif
  }
}

/*
  By: Owen Lyke
  SparkFun Electronics
  Date: Aug 27, 2019
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.
  ***********************************************************************************
  paulvha/ February 2020 / version 1.0
   *initial version

  paulvha / February 2020 / version 1.1
  * updated timer handling

  paulvha / March 2020 / version 2.1
  * updated to handle security / hashing (auto detect)
  * added major/minor version number request/response
  * update to sent float number for temperature values and battery level
  * changed names of some files
  * changed the internal temperature calculation
  * change ADC pin request handling
  * 
  ************************************************************************************

  Based on the original sample (see below) this example will allow data exchange between
  Linux system running amdtp-service on Bluez. The client will take the initiative to
  sent a request/command which this server will handle and respond back to the client.

  Optional support for BME280 connected to 'qwuic' interface. You have to UNCOMMENT
  the line 103 !!!!!!!!!!!!!!!!!!!!.
  This has been tested with an Adafruit BME280. Parts of the code below are coming from
  Sparkfun BME280 library: https://github.com/sparkfun/SparkFun_BME280_Arduino_Library
  The expected I2C address is 0x77.

    BME280      QWUIC
    GND         GND
    VCC         3V3
    SCK         SCL
    SDI         SDA

  There are different DEBUG levels that can be set in ble_debug.h

  BLE_Debug : will show all the data and program flow
  BLE_SHOW_DATA : will only show the data received and sent

  Make sure to installed the amdtpc-client on linux system ( e.g. Raspberry Pi) or use the
  Artemis client amdtpc

  // THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  // AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  // IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  // ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  // LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  // CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  // SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  // INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  // CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  // ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  // POSSIBILITY OF SUCH DAMAGE.

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

#include "amdtp.h"
#include "apollo3.h"                  // needed for battery load resistor

// Up to 29 characters
#define BLE_PERIPHERAL_NAME "Artemis AMDTP BLE"

// Server version
#define MAJOR_SERVERVERSION 2         // new features implemented that require update to client
#define MINOR_SERVERVERSION 0         // bug fixes, better calculation / layout

// maximum length of reply / data message
#define MAXREPLY 100

// temperature offset correction (see odt document for information)
// SET TO ZERO TO DISABLE !!!
#define TempOffset 0.0

/////////////////////////////////////////////////////////////////////////////
// include BME280 as option, REMOVE the comments to include
////////////////////////////////////////////////////////////////////////////
//#define INCLUDE_BME280

#ifdef INCLUDE_BME280

  // sent altitude in meters (true) or feet (false)
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

// trim values (see chapter 6 in amdtps_arduino.odt in extras folder!)
bool  bMeasured;
float fCalibrationTemperature;
float fCalibrationVoltage;
float fCalibrationOffset; 
/***********************************************************************/
void setup() {

#ifdef BLE_SHOW_DATA
  SERIAL_PORT.begin(115200);
  delay(1000);
  SERIAL_PORT.printf("Apollo3 BLE AMDTP protocol. Compiled: %s\n", __TIME__);
#endif

#ifdef AM_DEBUG_PRINTF
  //
  // Enable printing to the console.
  //
  enable_print_interface();
#endif

#ifdef INCLUDE_BME280
  Wire.begin();

  if (mySensor.beginI2C() == false) //Begin communication over I2C
  {
#ifdef BLE_SHOW_DATA
    Serial.println("The BME280 did not respond. Please check wiring.");
#endif
  }
  else
    BmeDetected = true;
#endif //INCLUDE_BME280

  pinMode(LED_BUILTIN, OUTPUT);
  set_led_low();

  // get ADC information
  if (! GetAdc() ){
#ifdef BLE_SHOW_DATA
    Serial.println("Could not initialize ADC. on Hold");
#endif
    while(1){
      set_led_high();
      delay(1000);
      set_led_low();
      delay(1000);
    }
  }

  //
  // Configure the peripheral's advertised name:
  //
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
  //
  // if in chat mode
  //
  if (opt_chat) chat();

  trigger_timers();

  am_hal_interrupt_master_disable();

  //
  // Check to see if the WSF routines are ready to go to sleep.
  //
  if ( wsfOsReadyToSleep() )
  {
      am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
  }

  am_hal_interrupt_master_enable();
}

/*
 * This routine wl update the WSF timers on a regular base
 */

void trigger_timers()
{
  //
  // Calculate the elapsed time from our free-running timer, and update
  // the software timers in the WSF scheduler.
  //
  update_scheduler_timers();
  wsfOsDispatcher();            // start any handlers if event is pending on them

  //
  // Enable an interrupt to wake us up next time we have a scheduled event.
  //
  set_next_wakeup();
}

/*
 * This routine is called when data / cmd has been received from the client
 *
 * The case-values (like AMDTP_CMD_HELLO) needs to stay in sync with
 * the eAmdtpPktcmd_t definitions in BLE_amdtp.h
 */

void UserRequestReceived(uint8_t * buf, uint16_t len)
{
#ifdef BLE_SHOW_DATA
      SERIAL_PORT.printf("received command with length %d : ", len);
      for (uint16_t i = 0 ; i< len ; i++) SERIAL_PORT.printf("0x%X ", buf[i]);
      SERIAL_PORT.println();
#endif

    uint16_t value;
    int16_t ival;
    float fval;
    val[0] = buf[0];    // echo requested command
    *val_len = 0;       // set for zero optional data
    val_data = &val[2]; // set start of optional data

    trigger_timers();

    switch (buf[0]) {

      case AMDTP_CMD_START_TEST_DATA:
        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("Send test data");
        #endif

        send_test_data();
        break;

      case AMDTP_CMD_STOP_TEST_DATA:
        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("Stop test data");
        #endif

        rest_test_data();
        break;

      case AMDTP_CMD_HELLO:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("Received HELLO / wakeup from client");
        #endif
        break;

      case AMDTP_CMD_TURN_LED_ON:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("Turn Led on");
        #endif

        set_led_high();
        break;

      case AMDTP_CMD_TURN_LED_OFF:

        #ifdef BLE_SHOW_DATA
           SERIAL_PORT.println("Turn Led off");
        #endif

        set_led_low();
        break;

      case AMDTP_CMD_REQ_BATTERY_LEVEL:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("Read internal battery level");
        #endif

        // read battery percentage
        fval = read_battery_perc();
        float_to_byte(fval, val_data);
        *val_len = 4;

        break;

      case AMDTP_CMD_REQ_BATTERYLOAD_ON:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("set Batteryload");
        #endif

        MCUCTRL->ADCBATTLOAD_b.BATTLOAD = 1;
        break;

      case AMDTP_CMD_REQ_BATTERYLOAD_OFF:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("remove Batteryload");
        #endif

        MCUCTRL->ADCBATTLOAD_b.BATTLOAD = 0;
        break;

      case AMDTP_CMD_REQ_INTERNAL_TEMP_FRH:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("Read internal temperature in Fahrenheit");
        #endif

        // read internal temperature
        fval = read_Internal_temp(1);
        float_to_byte(fval, val_data);
        *val_len = 4;
        break;

      case AMDTP_CMD_REQ_INTERNAL_TEMP_CEL:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("Read internal temperature in Celsius");
        #endif

        // read internal temp
        fval = read_Internal_temp(2);
        float_to_byte(fval, val_data);
        *val_len = 4;

        break;

      case AMDTP_CMD_ADC:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.print("Read ADC pin :");
            SERIAL_PORT.println(buf[1]);
        #endif

        ival = read_analog_value(buf[1]);
        *val_data++ = buf[1];       // ADC channel read
        *val_data++ = ival >> 8;   // MSB
        *val_data = ival & 0xff;   // LSB   
        *val_len = 3;

        *val_len = 3;
        break;

      case AMDTP_CMD_CHAT:

        #ifdef BLE_SHOW_DATA
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

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.print("Digital read pin: ");
            SERIAL_PORT.println(buf[1]);
        #endif

        *val_data++ = buf[1];     // pin read
        *val_data++ = digitalRead(buf[1]);

        *val_len = 2;
        break;

      case AMDTP_CMD_PIN_HIGH:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.print("Set HIGH digital pin: ");
            SERIAL_PORT.println(buf[1]);
        #endif

        *val_data++ = buf[1];     // pin
        *val_len = 1;

        pinMode(buf[1], OUTPUT);
        digitalWrite(buf[1], HIGH);
        break;

      case AMDTP_CMD_PIN_LOW:

        #ifdef BLE_SHOW_DATA
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

          #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("Read BME280");
          #endif

          Read_BME280();
        }
        else {  //length is zero to be detected by client

          #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("Read BME280: NOT DETECTED");
          #endif
        }
#else
        #ifdef BLE_SHOW_DATA  //length is zero to be detected by client
            SERIAL_PORT.println("Read BME280: NOT ENABLED");
        #endif

#endif //INCLUDE_BME280
        break;

     case AMDTP_CMD_VERSION:
        
        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("Request Server Version");
        #endif

        *val_data++ = MAJOR_SERVERVERSION;
        *val_data++ = MINOR_SERVERVERSION;

        *val_len = 2;
        break;
        
     case AMDTP_CMD_CUSTOM1:      // repeat for other options

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println("AMDTP_CMD_CUSTOM1");
        #endif

        // Do something here....
        break;

     default:
        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.printf("Unknown request : 0x%x", buf[0] );
        #endif
        val[0] = 0;             // indicate no defined action for this request
        break;
    }

    SendReplyClient();
}


/**
 * perform a read on analog pin
 *
 * Tested on 1.0.30 software for the Apollo. There is an issue in
 * analogRead when providing a pin number that is not supporting analog.
 * This has been raised as issue (& solution )143 on github
 * Pending resolution in the final code a valid check on the pin is
 * performed here to prevent a deadlock.
 *
 * Return : -1 in case of invalid ADC PIN for this server baord else analog value
 *
 */

int16_t read_analog_value(uint8_t pinNumber)
{
    if (pinNumber > 49) return -1;
    
    // validate for correct analog pin
    uint8_t padNumber = ap3_gpio_pin2pad(pinNumber);
    
#ifdef BLE_SHOW_DATA
  SERIAL_PORT.printf("pinnumber: %d, padnumber: %d\r\n", pinNumber, padNumber);
#endif
    uint8_t indi;
    for ( indi = 0;indi < AP3_ANALOG_CHANNELS ; indi++)
    {
        if (ap3_analog_configure_map[indi].pad == padNumber){
             return (int16_t) analogRead(pinNumber);
        }
    }

    // error
    return -1;
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


#ifdef INCLUDE_BME280

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

#ifdef BLE_SHOW_DATA
  Serial.printf(" Temp: %2.2f%c ", ret, symbol);
#endif

  //****************** HUMIDITY *****************
  ret = mySensor.readFloatHumidity();
  float_to_byte(ret, val_data);
  val_data += 4;
  *val_len += 4;
#ifdef BLE_SHOW_DATA
  Serial.printf(", Humidity: %2.2f",ret);
#endif

  //****************** PRESSURE *****************
  ret = mySensor.readFloatPressure();
  float_to_byte(ret, val_data);
  val_data += 4;
  *val_len += 4;
#ifdef BLE_SHOW_DATA
  Serial.printf(" ,Pressure: %2.2f",ret);
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
#ifdef BLE_SHOW_DATA
   Serial.printf(" ,Alt: %2.2f%c ", ret, symbol);
#endif

}
#endif //INCLUDE_BME280

/**
 * read internal temperature value
 * @param v =
 *  1 = Fahrenheit
 *  2 = Celsius
 *  
 *  return temperature or zero in case of error
 *  
 *  See chapter 6 in amdtps_arduino.odt in extras folder !
 */

float read_Internal_temp(uint8_t v)
{
  float fVT[3], itt;
  
  analogReadResolution(14);                      // 14 bits ADC reading
  float tempadc = analogRead(ADC_INTERNAL_TEMP); // get ADC temperature ADC reading
  analogReadResolution(10);                      // back to default

  if (tempadc == 0) return 0;
  
  //
  // Convert and scale the temperature.
  // Temperatures are in Fahrenheit range -40 to 225 degrees.
  // Voltage range is 0.825V to 1.283V
  //  
  float fADCTempVolts = ((float) tempadc)  * 2.0F / 16384.0F;  // to internal voltage measured

  // use the original method (either the Apollo3 was calibrated or it was instructed to use the defaults)
  if (bMeasured || TempOffset == 0.0)
  {
    //
    // Compute the temperature.
    //
    itt  = fCalibrationTemperature;
    itt /= (fCalibrationVoltage - fCalibrationOffset);
    itt *= (fADCTempVolts - fCalibrationOffset);
  }
  else
  {
    // based on work described in the odt document part of this package apply alternative approach
    // MAKE SURE THE TEMPOFFSET IS UPDATED !!!
    itt = (fADCTempVolts - TempOffset) / 3.8 * 1000;  // 3.8mV/degree kelvin  
  }
   
  if (v == 1)   // if Fahrenheit
    return( ((itt -273.15f)* 180.0f / 100.0f) + 32.0f );

  else          // celsius
    return (itt -273.15f);
}

/**
 * read the Battery ADC level provide the percentage
 */

float read_battery_perc()
{
    uint16_t battadc = analogRead(ADC_INTERNAL_VCC_DIV3);

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

  /* turn into percentage */
  return (float) val *100 / 3.3f;
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

    #ifdef BLE_Debug
      SERIAL_PORT.printf("Failed to sent data for client\n");
    #endif
  }
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
  // Get the temperature trim values as recorded.
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
    Serial.printf("Error - release of the ADC instance failed.\n");
    return(false);
  }
  
  fCalibrationTemperature = fTrims[0];
  fCalibrationVoltage    = fTrims[1];
  fCalibrationOffset   = fTrims[2];
  bMeasured = fTrims[3] ? true : false;

#if (defined BLE_Debug) 
  Serial.printf("\n");
  Serial.printf("TRIMMED TEMP    = %.3f\r\n", fTrims[0],6);
  Serial.printf("TRIMMED VOLTAGE = %.3f\r\n", fTrims[1],6);
  Serial.printf("TRIMMED Offset  = %.3f\r\n", fTrims[2],6);
  Serial.printf("Note - these trim values are '%s' values.\r\n\n",
                       bMeasured ? "calibrated" : "uncalibrated default");
#endif 
  return(true);
}

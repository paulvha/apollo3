/*
  This example will allow data exchange between Linux system running amdtp-service on Bluez
  or another Apollo3 running amdtp_client.
   
  The client will take the initiative to sent a request/command which this server will handle 
  and respond back to the client. The protocol is based on the Ambiq Micro Data Transfer Profile.

  See the arduino_amdtp.odt documents
  
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
   
  paulvha / October 2020 / version 3.0
  * This now works on top of ArduinoBLE and Apollo3 version 2.0.1
  
  paulvha / December 2020 / version 3.1
  * update with different ACK and timing to improve stability
  * changed from getTempDegF to getTempDegC (new in 2.0.2)
  * CRC32 is now also define in Mbed. Included instruction in CRC32.c/ .h
  
  ************************************************************************************
  == BME280
  Optional support for BME280 connected to 'qwuic' interface OR sda/scl pins. 
  
  You have to UNCOMMENT the line 89 !!!!!!!!!!!!!!!!!!!!.
  
  This has been tested with an Adafruit BME280. Parts of the code below are coming from
  Sparkfun BME280 library: https://github.com/sparkfun/SparkFun_BME280_Arduino_Library
  The expected I2C address is 0x77.

    BME280      QWUIC
    GND         GND
    VCC         3V3
    SCK         SCL
    SDI         SDA

  == DEBUG OPTIONS
  There are different DEBUG levels that can be set in amdtp_common.h

  BLE_Debug : will show all the data and program flow
  BLE_SHOW_DATA : will only show the data received and sent

  == CLIENT Software
  Make sure to install the amdtpc-client on linux system (e.g. Raspberry Pi / ubuntu) or use the
  Artemis client amdtpc

  ********************************************************************************

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

#include <ArduinoBLE.h>
#include "amdtp_common.h"
#include "amdtp_bridge.h"
#include "apollo3.h"                  // needed for battery load resistor

// Server version
#define MAJOR_SERVERVERSION 3         // new features implemented that require update to client
#define MINOR_SERVERVERSION 1         // bug fixes, better calculation / layout

// maximum length of reply / data message
#define MAXREPLY 100

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

uint16_t TestCounter = 0;       // for testdata
bool opt_chat = false;          // indicator for  chat


// BLE constructor
BLEService AMDTP_Service(ATT_UUID_AMDTP_SERVICE);  // create service

// create characteristics and allow remote device to read and write
BLEStringCharacteristic RxChar(ATT_UUID_AMDTP_RX, BLERead | BLEWrite, 100);
BLEStringCharacteristic TxChar(ATT_UUID_AMDTP_TX, BLERead | BLENotify, 100);
BLEStringCharacteristic AckChar(ATT_UUID_AMDTP_ACK, BLEWrite | BLERead | BLENotify ,50);

void setup() {
  
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println(F("Starting Bluetooth"));
  
  BLE.debug(Serial);   // leave debug on HCI on for more stability
  
  // begin initialization
  if (!BLE.begin()) {
    Serial.println(F("starting BLE failed!"));
    while (1);
  }

  // set the local name peripheral advertises
  BLE.setLocalName(BLE_PERIPHERAL_NAME);
  
  // set the UUID for the service this peripheral advertises
  BLE.setAdvertisedService(AMDTP_Service);

  // add the characteristic to the service
  AMDTP_Service.addCharacteristic(RxChar);
  AMDTP_Service.addCharacteristic(TxChar);
  AMDTP_Service.addCharacteristic(AckChar);

  // add service
  BLE.addService(AMDTP_Service);

  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristic
  RxChar.setEventHandler(BLEWritten, RxChar_Received);
  RxChar.setValue("rrr");

  TxChar.setEventHandler(BLEWritten, TxChar_Received);
  TxChar.setValue("ttt");
  
  AckChar.setEventHandler(BLEWritten, AckChar_Received);
  AckChar.setValue("aaa");

  // initialize AMDTP
  amdtps_init();

#ifdef INCLUDE_BME280
  Wire.begin();

  if (mySensor.beginI2C() == false) //Begin communication over I2C
  {
#ifdef BLE_SHOW_DATA
    Serial.println("\rThe BME280 did not respond. Please check wiring.");
#endif
  }
  else
    BmeDetected = true;
#endif //INCLUDE_BME280

  // start advertising
  BLE.advertise();

  Serial.println(("\rBluetooth device active, waiting for connections..."));
  String address = BLE.address();
  Serial.print("local address ");  Serial.println(address);
  Serial.print("local name ");  Serial.println(BLE_PERIPHERAL_NAME);
}

void loop() {

  //
  // if in chat mode
  //
  if (opt_chat) chat();

  // poll for BLE events
  BLE.poll();
}

/*
 * This routine is called when data / cmd has been received from the host
 *
 * The case-values (like AMDTP_CMD_HELLO) needs to stay in sync with
 * the eAmdtpPktcmd_t definitions in BLE_amdtp.h or amdtp_bridge.h
 */

void UserRequestReceived(uint8_t * buf, uint16_t len)
{
#ifdef BLE_SHOW_DATA
      SERIAL_PORT.printf("\rReceived command with length %d : ", len);
      for (uint16_t i = 0 ; i< len ; i++) SERIAL_PORT.printf("0x%X ", buf[i]);
      SERIAL_PORT.println();
#endif

    int16_t ival;
    float fval;
    val[0] = buf[0];    // echo requested command
    *val_len = 0;       // set for zero optional data
    val_data = &val[2]; // set start of optional data

    switch (buf[0]) {

      case AMDTP_CMD_START_TEST_DATA:
        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rSend test data"));
        #endif

        send_test_data();
        break;

      case AMDTP_CMD_STOP_TEST_DATA:
        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rStop test data"));
        #endif

        rest_test_data();
        break;

      case AMDTP_CMD_HELLO:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rReceived HELLO / wakeup from client"));
        #endif
        break;

      case AMDTP_CMD_TURN_LED_ON:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rTurn Led on"));
        #endif

        set_led_high();
        break;

      case AMDTP_CMD_TURN_LED_OFF:

        #ifdef BLE_SHOW_DATA
           SERIAL_PORT.println("\rTurn Led off");
        #endif

        set_led_low();
        break;

      case AMDTP_CMD_REQ_BATTERY_LEVEL:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rRead internal battery level"));
        #endif

        // read battery percentage
        fval = read_battery_perc();
        float_to_byte(fval, val_data);
        *val_len = 4;

        break;

      case AMDTP_CMD_REQ_BATTERYLOAD_ON:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rset Batteryload"));
        #endif

        MCUCTRL->ADCBATTLOAD_b.BATTLOAD = 1;
        break;

      case AMDTP_CMD_REQ_BATTERYLOAD_OFF:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rremove Batteryload"));
        #endif

        MCUCTRL->ADCBATTLOAD_b.BATTLOAD = 0;
        break;

      case AMDTP_CMD_REQ_INTERNAL_TEMP_FRH:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rRead internal temperature in Fahrenheit"));
        #endif

        // read internal temperature
        fval = read_Internal_temp(1);
        float_to_byte(fval, val_data);
        *val_len = 4;
        break;

      case AMDTP_CMD_REQ_INTERNAL_TEMP_CEL:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rRead internal temperature in Celsius"));
        #endif

        // read internal temp
        fval = read_Internal_temp(2);
        float_to_byte(fval, val_data);
        *val_len = 4;
        break;

      case AMDTP_CMD_ADC:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.print(F("\rRead ADC pin :"));
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
        Serial.print(F("\n<<<<< "));
        Serial.println((char *) &buf[1]);

        // if received BYE stop chat and echo back
        // else chat a new line.
        if (len == 5) {
          if (strcmp((char *) &buf[1], "BYE") == 0) {
            strcpy((char *) &val_data,"BYE");
            *val_len = 3;
            Serial.println(F("\nChat cancelled"));
            break;
          }
        }

        Serial.print(F(">>>>> ")); 
        opt_chat = true;        // will be called from loop to enable bluetooth keep-alive
        return;

      case AMDTP_CMD_READ_PIN:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.print(F("\rDigital read pin: "));
            SERIAL_PORT.println(buf[1]);
        #endif

        *val_data++ = buf[1];     // pin read
        *val_data++ = digitalRead(buf[1]);

        *val_len = 2;
        break;

      case AMDTP_CMD_PIN_HIGH:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.print(F("\rSet HIGH digital pin: "));
            SERIAL_PORT.println(buf[1]);
        #endif

        *val_data++ = buf[1];     // pin
        *val_len = 1;

        pinMode(buf[1], OUTPUT);
        digitalWrite(buf[1], HIGH);
        break;

      case AMDTP_CMD_PIN_LOW:

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.print(F("\rSet LOW digital pin: "));
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
            SERIAL_PORT.println(F("\rRead BME280"));
          #endif

          Read_BME280();
        }
        else {  //length is zero to be detected by client

          #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rRead BME280: NOT DETECTED"));
          #endif
        }
#else
        #ifdef BLE_SHOW_DATA  //length is zero to be detected by client
            SERIAL_PORT.println(F("\rRead BME280: NOT ENABLED"));
        #endif

#endif //INCLUDE_BME280
        break;

     case AMDTP_CMD_VERSION:
        
        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rRequest Server Version"));
        #endif

        *val_data++ = MAJOR_SERVERVERSION;
        *val_data++ = MINOR_SERVERVERSION;

        *val_len = 2;
        break;
        
     case AMDTP_CMD_CUSTOM1:      // repeat for other options

        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.println(F("\rAMDTP_CMD_CUSTOM1"));
        #endif

        // Do something here....
        break;

     default:
        #ifdef BLE_SHOW_DATA
            SERIAL_PORT.printf("\rUnknown request : 0x%x", buf[0] );
        #endif
        val[0] = 0;             // indicate no defined action for this request
        break;
    }

    SendReplyClient();
}

/**
 * perform a read on analog pin
 */
int16_t read_analog_value(uint8_t pinNumber)
{
     return (int16_t) analogRead(pinNumber);
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
    else {
      val_data++;
      BLE.poll();     // make sure to keep polling BLE regular
    }
  }
}

/*
 * Send some test data
 * This is only send on request of the host
 */
void send_test_data()
{

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
  Serial.print("\rTemp: ");
  Serial.print(ret,2); 
  Serial.print(symbol);
#endif

  //****************** HUMIDITY *****************
  ret = mySensor.readFloatHumidity();
  float_to_byte(ret, val_data);
  val_data += 4;
  *val_len += 4;
#ifdef BLE_SHOW_DATA
  Serial.print(", Humidity: ");
  Serial.print(ret,2); 
#endif

  //****************** PRESSURE *****************
  ret = mySensor.readFloatPressure();
  float_to_byte(ret, val_data);
  val_data += 4;
  *val_len += 4;
#ifdef BLE_SHOW_DATA
  Serial.print(", Pressure: ");
  Serial.print(ret,2); 
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
  Serial.print(" ,Alt: ");
  Serial.print(ret,2); 
  Serial.println(symbol);
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
 */

float read_Internal_temp(uint8_t v)
{
  float temp_Deg = getTempDegC();       // computed temperature in deg celsius (new in 2.0.2)

  if (temp_Deg == 0) return 0;
     
  if (v == 1)   // if Fahrenheit
    return( (temp_Deg* 180.0f / 100.0f) + 32.0f );
  else          // celsius
    return (temp_Deg);
}

/**
 * read the Battery ADC level provide the percentage
 */

float read_battery_perc()
{
  uint16_t battadc = analogReadVCCDiv3();    // reads VCC across a 1/3 voltage divider

  /* The power supply / battery level is provided as devided by 3, so the real voltage * 3.
   * the reference voltage is set as 2.0 internal.
   * ADC is set to 10 bit to keep compatible with Arduino
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
  if (! AmdtpSendData(val, *val_len + 2) ) {

#ifdef BLE_Debug
    SERIAL_PORT.printf("\rFailed to sent data for client\n");
#endif
  }
}

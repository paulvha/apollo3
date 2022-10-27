/*
**********************************************************************************
* Short description
**********************************************************************************
  This is a BLE peripheral implementation that can receive messages from a
  central and sent data out using Notify. For many of the sensor implementation it will
  be sufficient, easy to use and will allow a very light weight central implementation.

  It will require a good working function of ArduinoBLE.
  For Sparkfun Apollo3 library 1.2.1 a special patched ArduinoBLe (lower case e) and
  ExactLE package needs to be installed

  February 2021 / paulvha / version 1.0
  - initial release

/************************************************************************************
 *** hardware connect
 ************************************************************************************
= BME280

  This has been tested with an Adafruit BME280. Parts of the code below are coming from
  Sparkfun BME280 library: https://github.com/sparkfun/SparkFun_BME280_Arduino_Library
  The expected I2C address is 0x77.

    BME280      QWUIC
    GND         GND
    VCC         3V3
    SCK         SCL
    SDI         SDA

   OR (depending on your BME280 board and connected to QWiic)

   BME280       I2C
    VIN         3v3
    3V3(OUT)
    GND         GND
    SCK         SCL
    SDO
    SDI         SDA
    CS


**********************************************************************************
!! BEFORE YOU START!!
!! In case you use the default ArduinoBLE and you want to change the BDADDR !!!!!
!! This change has already been applied on ArduinoBLe
**********************************************************************************

Before you can compile it requires a SMALL CHANGE  in ArduinoBLE/src/uility/HCI.h.

Around line 80 you will find :

private :
int sendCommand(uint16_t opcode, uint8_t plen = 0, void* parameters = NULL);

Now move the sendCommand() to the line ABOVE private, into the public area so we can call
this function from the sketch:

int sendCommand(uint16_t opcode, uint8_t plen = 0, void* parameters = NULL);
private:


**********************************************************************************
* USAGE
**********************************************************************************

  Use Example10_central_BME280 to read complete BME280 information

  For debug :

  This example demonstrates basic BLE server (peripheral) functionality for the Apollo3 boards.
  How to use this example:
    - Install the nRF Connect app on your mobile device (must support BLE bluetooth)
    - Make sure you select the correct board definition from Tools-->Board
    - Compile and upload the example to your board with the Arduino "Upload" button
    - In the nRF Connect app look for the device in the "scan" tab.
        (By default it is called "Artemis peripheral BME280 BLE" but you can change that. see 'BLE settings' below)
    - Once the device is found click "connect"
    - The GATT server will load with 5 services:
      - Generic Access
      - Generic Attribute
      - Unknown Service (UUID ending on 1014)
    - click on "Unknown Service"
    - it will show:
      -Unknown Characteristic (UUID starting 19b10011)
      -Unknown Characteristic (UUID starting 19b10012)
      -Descriptors

    - click on the multiple-downward arrows next to unknown characteristic (UUID starting 19b10012)
     - it will now show properties NOTIFY,READ
     - Descriptors : Notifications enable

    - click on the up arrow next to unknown characteristic (UUID starting 19b10011)

    - enter value 07 + send : will show values of the BME280 at unknown characteristic (UUID starting 19b10012)
      it will flash as the data is sent in mutiple packages and only the last package is shown like  (0x) 30, 30, "00")
*/

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

/***********************************************************************/
// Define here the structure for the data to exchange.
// This structure must be defined the same on the central and peripheral
/***********************************************************************/
struct data_to_exchange{
  // BME280 data
  float humidity;
  float pressure;
  float altitude;
  float temperature;
  uint8_t meter;       // if true altitude is in meter, else in feet
  uint8_t celsius;     // if true temperature is celsius, else fahrenheit
} p;

////////////////////////////////////////////////////////////////////////////
// BME280 setting
////////////////////////////////////////////////////////////////////////////

// sent altitude in meters (1) or feet (0)
uint8_t AltitudeInMeter = 1;

// sent temperature in Celsius (1) or Fahrenheit (0)
uint8_t TempInCelsius = 1;

// after interval in seconds between sending an update
#define SENDINTERVAL  30

// BME280 detected
bool BME280_Detected = false;

////////////////////////////////////////////////////////////////////////////
// BLE settings
////////////////////////////////////////////////////////////////////////////

#include <ArduinoBLE_P.h>
#include "utility/HCI.h"    // needed for sendcommand

// MAX Up to 29 characters for BLE name
const char BLE_PERIPHERAL_NAME[] = "Artemis peripheral BME280 BLE";

// define your new Bluetooth device address in NEW_APOLLO_BDADR in REVERSE order :
// {0x66, 0x55, 0x44, 0x33, 0x22, 0x11} will become device address 11:22:33:44:55:66
static uint8_t BLEMacAddress[6]= {0x66, 0x55, 0x44, 0x33, 0x22, 0x21};

// create the BME280 service
BLEService BME280Service("19B10010-E8F2-537E-4F6C-D104768A1214");

// create characteristic and allow remote device to read and write (RECEIVE)
BLEByteCharacteristic BME280rCharacteristic("19B10011-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

// create characteristic and allow remote device to get notifications (SEND max 100 bytes)
BLEStringCharacteristic BME280sCharacteristic("19B10012-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 100);

////////////////////////////////////////////////////////////////////////////
// program setting
////////////////////////////////////////////////////////////////////////////

// define serial
#define SERIAL_PORT Serial

// enable sketch debug (remove comments)
//#define SKETCH_SHOW_DATA

// enable HCI layer debug (remove comments)
//#define BLE_Debug

/////////////////////////////////////////////////////////////////////
//
//  NO NEED FOR CHANGES AFTER THIS POINT
//
////////////////////////////////////////////////////////////////////

#include <Wire.h>
#include "SparkFunBME280.h"
BME280 mySensor;

uint16_t Interval = 1;              // send when requested the first time
bool     EnableSend = true;         // start sending unless requested to stop
char     input[10];                 // keyboard input buffer
int      inpcnt = 0;                // keyboard input buffer length

void setup() {

  SERIAL_PORT.begin(115200);
  delay(1000);
  SERIAL_PORT.printf("Apollo3 BME280 peripheral. Compiled: %s\n\r", __TIME__);

  ///////////////////////////////////////////////////
  // BLE INIT
  ///////////////////////////////////////////////////
#ifdef BLE_Debug
  BLE.debug(SERIAL_PORT);         // enable display HCI commands
#endif

  // begin initialization
  if (!BLE.begin()) {
    SERIAL_PORT.println(F("starting BLE failed!\r"));
    while (1);
  }

  // set the local name peripheral advertises
  BLE.setLocalName(BLE_PERIPHERAL_NAME);

  // set new address
  // THIS MIGHT FAIL AS IT NEEDS AN ADJUSTMENT IN THE DEFAULT ArduinoBLE
  // SEE TOP OF SKETCH
  // THE CHANGE HAS ALREADY BEEN APPLIED IN ArduinoBLe (lower case e)
  if (! WriteNewBdAddr()){
    SERIAL_PORT.println(F("Failed to set new Device Address!\r"));
    while (1);
  }

  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(BME280Service);

  // add the characteristics to the service
  BME280Service.addCharacteristic(BME280rCharacteristic);
  BME280Service.addCharacteristic(BME280sCharacteristic);

  // add the service
  BLE.addService(BME280Service);

  BME280rCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();

  ///////////////////////////////////////////////////
  // BME280 INIT
  ///////////////////////////////////////////////////

  Wire.begin();

  if (mySensor.beginI2C() == false) // Begin communication over I2C
  {
    SERIAL_PORT.println(F("The BME280 did not respond. Please check wiring.\r"));
  }
  else
    BME280_Detected = true;

  SERIAL_PORT.println(F("Ready to go ...\r"));
  String address1 = BLE.address();
  SERIAL_PORT.print(F("local address "));  SERIAL_PORT.print(address1);
  SERIAL_PORT.print(F("\r\nlocal name ")); SERIAL_PORT.print(BLE_PERIPHERAL_NAME);
  SERIAL_PORT.print("\r\n");
}

void loop() {

  // This routine will update the timers on a regular base
  // make sure this called often as part of your program loop
  BLE.poll();

  // handle any keyboard input
  if (SERIAL_PORT.available()) {
    while (SERIAL_PORT.available()) handle_input(SERIAL_PORT.read());
  }

  // handle anything received from Central
  if (BME280rCharacteristic.written()) {
    CentralRequestReceived();
  }

  // if allowed to send BME280 update and connected.
  if (EnableSend && BLE.connected()) {

    // Interval for reading & sending BME280
    if (--Interval == 0 ){
      CreateSendData();
      Interval = SENDINTERVAL;
    }
    else {
      // wait 10 x 100mS while checking / triggering BLE
      for (byte i = 0; i < 10; i++){
        BLE.poll();
        delay(100);
      }
    }
  }
}

//***************************************************************
//*
//* handle local keyboard input
//*
//***************************************************************
void handle_input(char c)
{
  if (c == '\r') return;    // skip CR

  if (c != '\n') {          // act on linefeed
    input[inpcnt++] = c;
    if (inpcnt < 9 ) return;
  }

  input[inpcnt] = 0x0;

  handle_cmd((uint8_t) atoi(input));

  BLE.poll();       // keep BLE alive

  display_menu();

  // reset keyboard buffer
  inpcnt = 0;
}

void display_menu()
{
  SERIAL_PORT.println(F("1.  Request altitude in meters\r"));
  SERIAL_PORT.println(F("2.  Request altitude in feet\r"));
  SERIAL_PORT.println(F("3.  Request temperature in Celsius\r"));
  SERIAL_PORT.println(F("4.  Request temperature in Fahrenheit\r"));
  SERIAL_PORT.println(F("5.  Request STOP sending data\r"));
  SERIAL_PORT.println(F("6.  Request (re)START sending data\r"));
  SERIAL_PORT.println(F("7.  Request NOW sending data\r"));
}

/*
 * Act on command. either received from the local serial or received from the connected central
 */

enum key_input{
  REQUEST_METERS = 1,
  REQUEST_FEET,
  REQUEST_CELSIUS,
  REQUEST_FRHEIT,
  REQUEST_STOP,
  REQUEST_START,
  REQUEST_NOW
};

// could be entered on peripheral or central
void handle_cmd(uint8_t req)
{
  switch (req)
  {
    case REQUEST_METERS:
        SERIAL_PORT.print(F("Report in meters\r\n"));
        AltitudeInMeter = 1;
        break;

    case REQUEST_FEET:
        SERIAL_PORT.print(F("Report in feet\r\n"));
        AltitudeInMeter = 0;
        break;

    case REQUEST_CELSIUS:
        SERIAL_PORT.print(F("Report in Celsius\r\n"));
        TempInCelsius = 1;
        break;

    case REQUEST_FRHEIT:
        SERIAL_PORT.print(F("Report in Fahrenheit\r\n"));
        TempInCelsius = 0;
        break;

    case REQUEST_STOP:
        SERIAL_PORT.print(F("Stop sending\r\n"));
        EnableSend = false;
        break;

    case REQUEST_START:
        SERIAL_PORT.print(F("(re)Start sending\r\n"));
        EnableSend = true;
        Interval = 1;     // send at next turn
        break;

    case REQUEST_NOW:
        SERIAL_PORT.print(F("Send now\r\n"));
        CreateSendData();
        Interval = SENDINTERVAL; // next one after SENDINTERVAL if enabled
        break;

    default:
        SERIAL_PORT.printf("Unknown request, %d\r\n", req);
        break;
  }
}

/*
 * This routine is called when data has been received from the host / central
 * we only expect 1 BYTE.. but have a buffer for 20
 */
void CentralRequestReceived()
{
  int len = BME280rCharacteristic.valueLength();
  uint8_t buf[20];

  if (len > 20) len = 20;

  // obtain received value(s)
  BME280rCharacteristic.readValue(buf, len);

#ifdef SKETCH_SHOW_DATA
  SERIAL_PORT.print(F("Received command with length: "));
  SERIAL_PORT.print(len);
  SERIAL_PORT.print("\r\n");

  for (int i = 0 ; i< len ; i++) {
    SERIAL_PORT.print(" 0x");
    SERIAL_PORT.print(buf[i], HEX);
  }
  SERIAL_PORT.println();
#endif

  // only ONE byte is expected for this solution
  handle_cmd(buf[0]);
}

/**
 * Read the BME280 values into the structure and send
 */
void CreateSendData()
{
  if (! BME280_Detected) {
    SERIAL_PORT.println(F("No BME280 Detected\r"));
    memset(&p,0x0,sizeof(struct data_to_exchange));
    SendReplyCentral((uint8_t *) &p, sizeof(struct data_to_exchange));
    return;
  }

  if (TempInCelsius) p.temperature = mySensor.readTempC();
  else p.temperature = mySensor.readTempF();

  p.celsius = TempInCelsius;

  p.humidity  = mySensor.readFloatHumidity();
  p.pressure = mySensor.readFloatPressure() / 100;

  if (AltitudeInMeter) p.altitude = mySensor.readFloatAltitudeMeters();
  else p.altitude = mySensor.readFloatAltitudeFeet();

  p.meter = AltitudeInMeter;

#ifdef SKETCH_SHOW_DATA

  SERIAL_PORT.print(F("\r\nTemperature\t"));
  SERIAL_PORT.print(p.temperature);
  if (p.celsius) SERIAL_PORT.println("*C\r");
  else SERIAL_PORT.println(F("*F\r"));

  SERIAL_PORT.print(F("Humidity\t"));
  SERIAL_PORT.print(p.humidity);
  SERIAL_PORT.println(F("%\r"));

  SERIAL_PORT.print(F("Pressure\t"));
  SERIAL_PORT.print(p.pressure);
  SERIAL_PORT.println(F(" hPa\r"));

  SERIAL_PORT.print(F("Altitude\t"));
  SERIAL_PORT.print(p.altitude);
  if (p.meter) SERIAL_PORT.println(F(" Meter\r"));
  else SERIAL_PORT.println(F(" Feet\r"));

#endif

  SendReplyCentral((uint8_t *) &p, sizeof(struct data_to_exchange));
}

/**
 * sent a data message to the central/host
 *
 * Format received data :
 * buf =  data to be sent
 * len  = length of data
 *
 * We have a challenge that the characteristic is a STRING format, so we can not
 * have a value 0x0 in the data to be sent. Hence we convert all
 * the values to ascii and send as ascii characters.
 * Of course we have to do the opposite on the central side
 *
 * As we send over notify the max length is the MTU-length(often the default 23)
 * 3 bytes are used for handles so we keep it to max 20
 * Of course we have to do the opposite on the central side
 */
bool SendReplyCentral(uint8_t *buf, uint8_t len)
{
  String ToSend = "";   // string to send
  int i, j;             // conversion counter
  char c[5];            // conversion

  if (!BLE.connected()) {
    SERIAL_PORT.print(F("Error: Not connected \r\n"));
    return(false);
  }

  ToSend.concat((char) (len* 2)); // first byte of new message in the total data length

  // convert to ascii and add to string
  for (i = 0, j = 1; i < len ; i++){
    sprintf(c,"%02X",buf[i]);
    ToSend.concat(c);
    j += 2;

    // As we send over notify the max length is the MTU (often 23)
    // 3 bytes are used for handles so we keep it to max 20
    if (j > 18) {
      BME280sCharacteristic.writeValue(ToSend);
      ToSend = "";
      j=0;
    }
  }

  // send last packet (if pending)
  if (j != 0) BME280sCharacteristic.writeValue(ToSend);

  return(true);
}

/*
 * Most of the Apollo3 chips have the same bluetooth address
 * this will allow you to change that.
 *
 * See top of sketch for change to apply to make this work !!!!
 *
 * Write new address with vendor specific command
 */
bool WriteNewBdAddr()
{
  return(true);
  // NATIONZ
  //int result = HCI.sendCommand(0xFC32, 6, BLEMacAddress);

  //if (result == 0) return true;

//  return false;
}

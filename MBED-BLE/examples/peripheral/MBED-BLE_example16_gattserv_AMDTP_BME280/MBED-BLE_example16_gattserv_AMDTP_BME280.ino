/* This is a peripheral for the AMDTP / BME280 monitor

  It works in conjunction with the peripheral MBED-BLE_example16_gattclient_AMDTP_BME280

  This is a example of using AMD Transfer Protocol (AMDTP) service. With AMDTP you can send larger
  data sizes (by default up to 512 bytes). This data will be broken down by the sender in smaller 
  packages to the size of the MTU. The smaller pacakages will be controlled exchanged between sender and 
  receiver with the use of the AMDTP. On the receiver side these package will be combined 
  into a single data package and the end-to-end CRC will be checked to make sure the data is correct.

  In this example we exchange only a small data package: The size of the BME280 data structure. 
  The example will show a menu and the client can send instructions to the server/peripheral to
  change the data on the server to be send to the client/central.  

  The AMDTP is based on the source code that is provided with AmbiqSuiteSDK. More information is available
  in the extras folder of the library.

  The sketch and connection to the server / peripheral is partly based on
  https://os.mbed.com/teams/mbed-os-examples/code/mbed-os-example-ble-GattClient/file/71d7cec222eb/main.cpp/
  
  But has many more features added

  Version 1.0 / January 2022 / paulvha
  - initial release

/************************************************************************************
 *** hardware connection
 ************************************************************************************
  BME280

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
 *****************************************************************************************
 *  Parts taken from
 *  https://github.com/ARMmbed/mbed-os-example-ble/tree/master/BLE_GattServer_AddService
 *  mbed Microcontroller Library
 *
 * Copyright (c) 2006-2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************************
 * USAGE
 ****************************************************************************************

  It works in conjunction with the central MBED-BLE_example16_gattclient_AMDTP_BME280, using
  Artemis/Apollo3/Mbed/Cordio BLE to read complete BME280 information.
  There is also a client version that works on top of bleak and part of this library.
  
*/

////////////////////////////////////////////////////////////////////////////
// BME280 setting
////////////////////////////////////////////////////////////////////////////


/***********************************************************************/
// Define here the structure for the data to exchange.
// This structure must be defined the same on the central and peripheral
/***********************************************************************/
struct data_to_exchange {
  // BME280 data
  float humidity;
  float pressure;
  float altitude;
  float temperature;
  uint8_t meter;          // if true altitude is in meter, else in feet
  uint8_t celsius;        // if true temperature is celsius, else fahrenheit
  
  // command feedback
  int8_t CmdStat;          // echo of earlier send commmand or -1 in case of error
  uint8_t MagicNumber;     // magic number
};
#define MAGIC_CMD 0xCF      // indicate that notify data is command response

struct data_to_exchange p;  // store BME280 data

// send altitude in meters (1) or feet (0)
uint8_t AltitudeInMeter = 1;

// send temperature in Celsius (1) or Fahrenheit (0)
uint8_t TempInCelsius = 1;

// after interval in seconds between sending an update
#define SENDINTERVAL  30

// BME280 detected
bool BME280_Detected = false;

// enable / disable showing values locally
bool show = true;

// remote request to send NOW
bool SendNow = false;

///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "amdtps.h"
#include <Wire.h>
#include "SparkFunBME280.h" //Click here to get the library: http://librarymanager/All#SparkFun_BME280

// Input check in milliseconds
static const std::chrono::milliseconds CHECK_RATE_MS = 450ms;

// name of device
// this can be used on the Artemis client to find and connect.
// on the bleak client the MAC address needs to be used.
char _device[20]= "MBED_BME280";

/*Commands either received from the local serial or received from the connected central*/
enum key_input{
  REQUEST_METERS = 1,
  REQUEST_FEET,
  REQUEST_CELSIUS,
  REQUEST_FRHEIT,
  REQUEST_STOP,
  REQUEST_START,
  REQUEST_NOW,
  REQUEST_SHOW
};

//////////////////////////////////////////////////////////////////////////////////////////
// NO CHANGE NEEDED BEYOND THIS POINT
/////////////////////////////////////////////////////////////////////////////////////////
// Sensor Constructor
BME280 mySensor;

// event handler
static events::EventQueue event_queue(/* event count */ 16 * EVENTS_EVENT_SIZE);

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

// the Gattserver constructor
GattServAMDTP BmeSvc(ble_instance, event_queue);

#define BUFLEN 10
uint16_t Interval = SENDINTERVAL;   // send when time-out
bool     EnableSend = true;         // start sending unless requested to stop
char     input[BUFLEN];             // keyboard input buffer
int      inpcnt = 0;                // keyboard input buffer length
unsigned long StartTime = 0;        // determine BME280 start time

uint8_t StoreBuf[BUFLEN];           // store central received data
uint8_t StoreBufLen =0 ;            // store length of received data
bool DataReceived = false;          // indicate that data was received from Central

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(500); }

  flush();
  Serial.println("\r\nExample16 : Gatt Server AMDTP with BME280\r\nPress Enter to start or wait 2 seconds\r");
  for(uint8_t i = 0 ; i < 20; i++) {
    if (Serial.available()) break;
    delay(100);
  }

  flush();

#ifdef BLE_Debug
  Enable_BLE_Debug(true);
#endif

  // set the advertise device name
  BmeSvc.set_device_name(_device);
 
  // set handler to call to process events
  ble_instance.onEventsToProcess(schedule_ble_events);

  // start BME280
  Wire.begin();

  if (mySensor.beginI2C() == false) // Begin communication over I2C
  {
    printf("The BME280 did not respond. Please check wiring.\r\n");
  }
  else{
    printf("The BME280 detected.\r\n");
    BME280_Detected = true;
  }

  // add loop to check as a recurring event on the event queue
  event_queue.call_every(CHECK_RATE_MS, &Check_loop);

  // initialise the AMD transfer protocol service
  BmeSvc.start();
}

void loop() {
  // put your main code here, to run repeatedly:
  // moved to Check_loop()
}

/* flush any pending input*/
void flush()
{
 delay(100);

 while (Serial.available()) {
  Serial.read();
  delay(50);
 }
}

/* Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
  event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

/* called regular as defined in setup() */
void Check_loop()
{
  // handle any keyboard input
  if (Serial.available()) {
    while (Serial.available()) {
      handle_input(Serial.read());
      delay(50);
    }
  }

  // handle central command received
  if (DataReceived){
    HandleDataReceived();
  }

  // if allowed to send BME280 update and connected.
  if (EnableSend && BmeSvc.IsConnected()) {

    // Interval for reading & sending BME280
    if (millis() - StartTime > (Interval * 1000)) {
      update_BME280_value();
      Interval = SENDINTERVAL;
      StartTime = millis();
    }
  }
  else
    StartTime = millis();
}

/* take action on received command from central/client */
void HandleDataReceived()
{
/*
  // debug only
  printf("len = %d\r\n", StoreBufLen);
  for (uint8_t i; i < StoreBufLen ; i++) printf("%x %d\r\n",StoreBuf[i], StoreBuf[i]);
*/
  handle_cmd(StoreBuf[0], false); // false indicates it came from remote
  DataReceived = false;
}

/* handoff received data from central/client */
void StoreDataReceived(uint8_t *data, uint16_t len)
{
  if (len > 0){
    StoreBufLen  = len;
    if (StoreBufLen > BUFLEN)  StoreBufLen = BUFLEN;
    for (uint8_t i; i < StoreBufLen ; i++) StoreBuf[i] = data[i];
    DataReceived = true;    // indicate we have new data received
  }
}

/* handle local keyboard input */
void handle_input(char c)
{
  if (c == '\r') return;    // skip CR

  if (c != '\n') {          // act on linefeed
    input[inpcnt++] = c;
    if (inpcnt < BUFLEN -1 ) return; // prefend overflow
  }

  input[inpcnt] = 0x0;

  handle_cmd((uint8_t) atoi(input), true);

  display_menu();

  // reset keyboard buffer
  inpcnt = 0;
}

void display_menu()
{
  Serial.println();
  Serial.println(F("1.  Request altitude in meters\r"));
  Serial.println(F("2.  Request altitude in feet\r"));
  Serial.println(F("3.  Request temperature in Celsius\r"));
  Serial.println(F("4.  Request temperature in Fahrenheit\r"));
  Serial.println(F("5.  Request STOP sending data\r"));
  Serial.println(F("6.  Request (re)START sending data\r"));
  Serial.println(F("7.  Request NOW sending data\r"));
  if(show)
    Serial.println(F("8.  Stop showing BME280 values\r"));
  else
    Serial.println(F("8.  Start showing BME280 values\r"));

  Serial.println(F("     Or just wait\r"));
}

/*
 * Handle command which could be entered on peripheral or central
 *  @param local : coming from local = true, else remote
 */
void handle_cmd(uint8_t req, bool local)
{
  // save for potential sending client/central
  uint8_t BmeCmd = req;
  bool SendNow = false;
  
  switch (req)
  {
    case REQUEST_METERS:
        printf("Report in meters\r\n");
        AltitudeInMeter = 1;
        break;

    case REQUEST_FEET:
        printf("Report in feet\r\n");
        AltitudeInMeter = 0;
        break;

    case REQUEST_CELSIUS:
        printf("Report in Celsius\r\n");
        TempInCelsius = 1;
        break;

    case REQUEST_FRHEIT:
        printf("Report in Fahrenheit\r\n");
        TempInCelsius = 0;
        break;

    case REQUEST_STOP:
        printf("Stop sending\r\n");
        EnableSend = false;
        break;

    case REQUEST_START:
        printf("(re)Start sending\r\n");
        EnableSend = true;
        Interval = 1;             // send at next turn
        break;

    case REQUEST_NOW:
        printf("Send now\r\n");
        SendNow = true;            // send it
        break;

    case REQUEST_SHOW:           // only expected locally
        if (local) {
          if (show) show = false;
          else show = true;
        }
        break;

    default:
        printf("Unknown request, %d\r\n", req);
        BmeCmd = -1;           // indicate error in request
        break;
  }

  // only send when command was received from remote & not send now BME280 info
  if(! local && ! SendNow) {
    memset(&p, 0, sizeof(struct data_to_exchange));
    p.CmdStat = BmeCmd;
    p.MagicNumber = MAGIC_CMD;
    BmeSvc.SendToCentral((uint8_t *) &p, sizeof(struct data_to_exchange));    // sent command
  }

  // in case update now request
  if (SendNow){
      //event_queue.call_in(CHECK_RATE_MS, &update_BME280_value);
      update_BME280_value();
  }
}

/* Read the BME280 values into the structure and send */
void update_BME280_value()
{
  if (! BME280_Detected) {
    printf("No BME280 Detected\r");
    memset(&p,0x0,sizeof(struct data_to_exchange));
    BmeSvc.SendToCentral((uint8_t *) &p, sizeof(struct data_to_exchange));    // sent command
    return;
  }
  
  p.MagicNumber = 0x0;  // indicate BME280 data
  
  if (TempInCelsius) p.temperature = mySensor.readTempC();
  else p.temperature = mySensor.readTempF();

  p.celsius = TempInCelsius;

  p.humidity  = mySensor.readFloatHumidity();
  p.pressure = mySensor.readFloatPressure() / 100;

  if (AltitudeInMeter) p.altitude = mySensor.readFloatAltitudeMeters();
  else p.altitude = mySensor.readFloatAltitudeFeet();

  p.meter = AltitudeInMeter;

  if (show){
    Serial.print(F("\r\nTemperature\t"));
    Serial.print(p.temperature);
    if (p.celsius) Serial.println("*C\r");
    else Serial.println(F("*F\r"));

    Serial.print(F("Humidity\t"));
    Serial.print(p.humidity);
    Serial.println(F("%\r"));

    Serial.print(F("Pressure\t"));
    Serial.print(p.pressure);
    Serial.println(F(" hPa\r"));

    Serial.print(F("Altitude\t"));
    Serial.print(p.altitude);
    if (p.meter) Serial.println(F(" Meter\r"));
    else Serial.println(F(" Feet\r"));
  }

  if ( BmeSvc.IsConnected() ) {
    BmeSvc.SendToCentral((uint8_t *) &p, sizeof(struct data_to_exchange));
  }
  else {
    printf("Can not send to client. Not Connected \r\n");
  }
}

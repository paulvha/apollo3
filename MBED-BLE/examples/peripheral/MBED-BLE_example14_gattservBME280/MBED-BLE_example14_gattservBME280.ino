/* This is a peripheral for the BME280 monitor

  This is a BLE peripheral implementation that can sent data out using Notify.
  The maximum size is 20 bytes but for many of the sensor implementation it will
  be sufficient, easy to use and will allow a very light weight central implementation.

  It will also accept command from the central to adjust the notify data. Same as local menu.
  Feedback about the received command will be sent back using Notify indicated with a special MagicNumber

  It works in conjunction with the central MBED-BLE_example14_gattclientBME280, using
  Artemis/Apollo3/Mbed/Cordio BLE. There is also a client version (Python_bleak_BME280) 
  that works on top of bleak and part of this library.Python_bleak_BME280

  January 2022 / paulvha / version 1.0
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

  It works in conjunction with the central MBED-BLE_example16_gattclientBME280, using
  Artemis/Apollo3/Mbed/Cordio BLE to read complete BME280 information.
  There is also a client version that works on top of bleak and part of this library.

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

////////////////////////////////////////////////////////////////////////////
// BME280 setting
////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "gatt_server_bme280.h"

#include <Wire.h>
#include "SparkFunBME280.h" //Click here to get the library: http://librarymanager/All#SparkFun_BME280
BME280 mySensor;

// event handler
static events::EventQueue event_queue(/* event count */ 16 * EVENTS_EVENT_SIZE);

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

// the Gattserver constructor
GattServBME BmeSvc(ble_instance, event_queue);

// Input check in milliseconds
static const std::chrono::milliseconds CHECK_RATE_MS = 450ms;

// name of device
// this can be used on the Artemis client to find and connect.
// on the bleak client the MAC address needs to be used.
char _device[20]= "MBED_BME280";

#define BUFLEN 10
uint16_t Interval = SENDINTERVAL;   // send when time-out
bool     EnableSend = true;         // start sending unless requested to stop
char     input[BUFLEN];             // keyboard input buffer
int      inpcnt = 0;                // keyboard input buffer length
unsigned long StartTime = 0;        // determine BME280 start time

uint8_t StoreBuf[BUFLEN];           // store central received data
uint8_t StoreBufLen =0 ;            // store length of received data
bool DataReceived = false;          // indicate that data was received from Central

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

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(500); }

  flush();
  Serial.println("\r\nExample16 : Gatt Server BME280\r\nPress Enter to start or wait 2 seconds\r");
  for(uint8_t i =0; i < 20; i++) {
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
  handle_cmd(StoreBuf[0], false); // false = remote
  DataReceived = false;
}

/* handoff received data from central/client */
void StoreDataReceived(const uint8_t *data, uint8_t len)
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
  _BmeCmd = req;

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
        Interval = 1;             // send at next turn
        break;

    case REQUEST_SHOW:           // only expected locally
        if(local) show != show;
        break;

    default:
        printf("Unknown request, %d\r\n", req);
        _BmeCmd = -1;           // indicate error in request
        break;
  }

  // only send when command was receied from remote
  if(!local) BmeSvc.SendToCentral(false);    // sent command
}

/* Read the BME280 values into the structure and send */
void update_BME280_value()
{
  if (! BME280_Detected) {
    printf("No BME280 Detected\r");
    memset(&p,0x0,sizeof(struct data_to_exchange));
    BmeSvc.SendToCentral();
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

  BmeSvc.SendToCentral(true);
}

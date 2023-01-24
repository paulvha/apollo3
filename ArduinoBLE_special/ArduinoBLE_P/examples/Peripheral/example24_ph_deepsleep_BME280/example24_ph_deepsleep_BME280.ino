/*
**********************************************************************************
* Short description
**********************************************************************************
  This example will show how to use deepsleep with BLE and BME280. There are a number
  of issues to consider and take in account.
  This is a BLE peripheral implementation that can receive messages from a
  central and sent data out using Notify. For many of the sensor implementation it will
  be sufficient, easy to use and will allow a very light weight central implementation.

  Be aware that you can only send as many bytes as the size of the MTU. Default size
  is 20 ( 23 - 3 overhead), but it could have been agreed different after connect.
  see example13.

  It will require a good working function of ArduinoBLE.

**********************************************************************************
* versioning
**********************************************************************************
  January 2023 / paulvha / version 1.1.2
  - initial release
  - Example24 based on Example21 V1.1.2 with deepsleep option
  - It now uses the BME280 with SPI to communicate instead of Wire

  In case of V1.2.3 Sparkfun Artemis Library.
  ==========================================
  For Sparkfun Apollo3 library V1.2.3 a special patched ArduinoBLE_P (P = patched) and
  ExactLE HCI package needs to be installed. See installation instructions.
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  In case of V2.2.1 Sparkfun Artemis Library.
  ==========================================
  MAKE SURE TO READ THE INCLUDED Artemins_DEEPSLEEP.TXT DOCUMENT that is part of this example!!!!!!!!!!!!!!!!!!!
  It explains why this deepsleep will fail, unless you perform extra changes !

  Be aware that SPI can not handle deepsleep "out of the box" changes need to apply in SPI.end(). 
  For matter and the fact that it does not handle MSBFIRST / LSBFIRST correctly 
  (see https://github.com/sparkfun/Arduino_Apollo3/issues/478) an updated version of SPI.cpp and SPI.h 
  is included in the src.zip. This folder should replace the src-folder in  the folder 
  SparkFun/hardware/apollo3/2.2.1/libraries/SPI

************************************************************************************
* Hardware connect
************************************************************************************
= BME280

  This has been tested with an Adafruit BME280. 

   BME280       SPI on board
    VIN         3v3
    3V3(OUT)
    GND         GND
    SCK         SCK
    SDO         MISO
    SDI         MOSI
    CS          CS  (defined below)
  
  Parts of the code below are coming from
  Sparkfun BME280 library: https://github.com/sparkfun/SparkFun_BME280_Arduino_Library

**********************************************************************************
* USAGE
**********************************************************************************

  Use Example21_central_BME280 to read complete BME280 information. The central is the 
  same, just this example24 peripheral is using SPI instead of I2C for BME280.

  January 2023 : there is now also an Android App available.
  
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

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
// OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
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
  byte meter;       // if true altitude is in meter, else in feet
  byte celsius;     // if true temperature is celsius, else fahrenheit
} p;

// To improve synchronization there are 2 checks MAGICNUM in byte 0 and length in byte 1
#define MAGICNUM 0XCF  // should be byte 0 in new message

////////////////////////////////////////////////////////////////////////////
// BME280 setting
////////////////////////////////////////////////////////////////////////////

// send altitude in meters (1) or feet (0)
byte AltitudeInMeter = 1;

// send temperature in Celsius (1) or Fahrenheit (0)
byte TempInCelsius = 1;

// FIRST interval in seconds after connect
#define FIRST_SENDINTERVAL  10

// Interval in seconds before sending a next update
#define SENDINTERVAL  30

// BME280 detected
bool BME280_Detected = false;

////////////////////////////////////////////////////////////////////////////
// Program setting
////////////////////////////////////////////////////////////////////////////

// enable sketch data (disable : comment out)
#define SKETCH_SHOW_DATA

// enable HCI layer debug (remove comments)
//#define BLE_Debug

// uptime in minutes
// if zero (0) sleep will be NOT be set based on time
// you can use the menu option to go to deepsleep
#define UPTIME  5

// downtime / sleep in minutes
#define DOWNTIME 1

// In case this is run on V1.x Sparkfun library, there are additional steps needed to install ArduinoBLE_P. make sure to follow those !!
#if defined (ARDUINO_ARCH_APOLLO3) && ! defined (ARDUINO_ARCH_MBED)
#define VERSION_1 1
#endif

// define serial
#define SERIAL_PORT Serial

////////////////////////////////////////////////////////////////////////////
// SPI pin setting
////////////////////////////////////////////////////////////////////////////
// 
// The MOSI, MISO, SCK pins for your board where the BME280 is connected need to be defined
// For V2 library these pins will not be disabled before sleep and pins will used to repower 
// to select the right SPI-IOM after sleep.
// For most boards these are already defined as part of the variant definition

#if !defined MOSI || !defined MISO || !defined SCK
#error MOSI or MISO not defined. Check the board variant definition.
#endif 

// chip select 
// !! This will fail with an Artemis Micromod as CS is defined as part of the variant
// comment out the line below in that case
#define CS  10

/////////////////////////////////////////////////////////////////////
//
//  NO NEED FOR CHANGES AFTER THIS POINT
//
////////////////////////////////////////////////////////////////////

#ifdef VERSION_1
#include <ArduinoBLE_P.h>
#else
#include <ArduinoBLE.h>
#endif

#include <SPI.h>
#include "SparkFunBME280.h"
#include "RTC.h"              // Include RTC library included with the Arduino_Apollo3 core

// MAX up to 29 characters for BLE name
const char BLE_PERIPHERAL_NAME[] = "Peripheral BME280 BLE";

// create the BME280 service
BLEService BME280Service("19B10010-E8F2-537E-4F6C-D104768A1214");

// create characteristic and allow remote device to read and write (RECEIVE)
BLEByteCharacteristic BME280rCharacteristic("19B10011-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

// create characteristic and allow remote device to get notifications (SEND max 100 bytes)
BLECharacteristic BME280sCharacteristic("19B10012-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 100);

BME280 mySensor;

#ifdef VERSION_1
APM3_RTC rtc;                 // in V2 this is defined in the RTC.h
#endif

uint16_t Interval;            // Interval time between sending auto-update
bool     EnableSend = true;   // start sending unless requested to stop
char     input[10];           // keyboard input buffer
int      inpcnt = 0;          // keyboard input buffer length
unsigned long st;             // need for uptime

void setup() {

  SERIAL_PORT.begin(115200);
  delay(1000);
  SERIAL_PORT.printf("Example24 BME280 (SPI) peripheral & deepsleep. Compiled: %s\n\r", __TIME__);

  // start BME280
  StartBME280();

  // Start BLE
  StartBLE();
}

void loop() {
  //
  // continue to loop for UPTIME minutes
  //
  if (UPTIME > 0) {
    if ( millis() - st > (UPTIME*1000*60) ) PeripheralSleep();
  }
  
  // This routine will trigger the HCI-layer flow
  // make sure this called often as part of your program loop
  //
  BLE.poll();
  
  //
  // handle any keyboard input
  //
  if (SERIAL_PORT.available()) {
    while (SERIAL_PORT.available()) handle_input(SERIAL_PORT.read());
  }

  //
  // handle anything received from Central
  //
  if (BME280rCharacteristic.written()) {
    CentralRequestReceived();
  }

  //
  // if allowed to send BME280 update and connected.
  //
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

/**
 * start BME280
 */
void StartBME280()
{  
  SERIAL_PORT.println("\nStarting BME280");
  BME280_Detected = false;
  
  if (mySensor.beginSPI(CS) == false) // Begin communication over SPI
  {
    SERIAL_PORT.println(F("The BME280 did not respond. Please check wiring.\r"));
  }
  else {
    BME280_Detected = true;
    SERIAL_PORT.println(F("BME280 has been detected."));
  }
}

/**
 * Start BLE
 */
void StartBLE()
{
  SERIAL_PORT.println("Starting BLE");

#ifdef BLE_Debug
  BLE.debug(SERIAL_PORT);         // enable display HCI commands on ArduinoBLE_P
#endif

  // begin initialization
  if (!BLE.begin()) {
    SERIAL_PORT.println(F("Starting BLE failed! Freeze\r"));
    while (1);
  }

  // set the local name peripheral advertises
  BLE.setLocalName(BLE_PERIPHERAL_NAME);

  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(BME280Service);

  // add the characteristics to the service
  BME280Service.addCharacteristic(BME280rCharacteristic);
  BME280Service.addCharacteristic(BME280sCharacteristic);

  // add the service
  BLE.addService(BME280Service);

  BME280rCharacteristic.writeValue(0);

  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // start advertising
  BLE.advertise();

  st = millis();

  Interval = FIRST_SENDINTERVAL;      // reset

  SERIAL_PORT.println(F("Ready to go ...\r"));
  String address1 = BLE.address();
  SERIAL_PORT.print(F("local address "));  SERIAL_PORT.print(address1);
  SERIAL_PORT.print(F("\r\nlocal name '")); SERIAL_PORT.print(BLE_PERIPHERAL_NAME);
  SERIAL_PORT.print("'\r\n");
}

/**
 * handle local keyboard input
 */
void handle_input(char c)
{
  if (c == '\r') return;    // skip CR

  if (c != '\n') {          // act on linefeed
    input[inpcnt++] = c;
    if (inpcnt < 9 ) return;
  }

  input[inpcnt] = 0x0;

  handle_cmd((uint8_t) atoi(input));

  BLE.poll();             // keep BLE alive

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
  SERIAL_PORT.println(F("8.  Request DeepSleep for peripheral\r"));
}

/**
 * Act on command. either received from the local serial or received from the connected central
 */
enum key_input{
  REQUEST_METERS = 0x1,
  REQUEST_FEET,
  REQUEST_CELSIUS,
  REQUEST_FRHEIT,
  REQUEST_STOP,
  REQUEST_START,
  REQUEST_NOW,
  REQUEST_SLEEP
};

/** 
 * could be entered on peripheral or central
 */
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
      SERIAL_PORT.print(F("Sending BME280 info now\r\n"));
      CreateSendData();
      Interval = SENDINTERVAL; // next one after SENDINTERVAL if enabled
      break;

    case REQUEST_SLEEP:
      SERIAL_PORT.print(F("Set peripheral to sleep now\r\n"));
      PeripheralSleep();
      break;
        
    default:
      SERIAL_PORT.printf("Unknown request, %d\r\n", req);
      break;
  }
}

/**
 * This routine is called when data has been received from the host / central
 * we only expect 1 BYTE.. but have a buffer for 20.. who knows in the future
 */
void CentralRequestReceived()
{
  int len = BME280rCharacteristic.valueLength();
  uint8_t buf[20];

  if (len > 20) len = 20;

  // obtain received value(s)
  BME280rCharacteristic.readValue(buf, len);

/** set to 1 to enable debug */
#if 0
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
  handle_cmd(buf[0] - '0');  // ascii to hex
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
 * Format data :
 * buf =  data to be sent
 * len  = length of data
 *
 * As we send over notify the max length is the MTU-length(often the default 23)
 * 3 bytes are used for handles so we keep it to max 20
 * Of course we have to do the opposite on the central side to combine
 */
bool SendReplyCentral(uint8_t *buf, uint8_t len)
{
  uint8_t ToSend[25];
  int i, j;             // conversion counter

  if (!BLE.connected()) {
    SERIAL_PORT.print(F("Error: Not connected (anymore)\r\n"));
    return(false);
  }

  ToSend[0] = MAGICNUM; // magicnumber to improve synchronisation
  ToSend[1] = len;      // second byte of new message in the total data length

  // send in blocks
  for (i = 0, j = 2; i < len ; i++){
    ToSend[j++] = buf[i];
    // As we send over notify the max length is the MTU (often 23)
    // 3 bytes are used for handle so we keep it to max 20
    if (j == 20) {
      BME280sCharacteristic.writeValue(ToSend, j);
      j=0;
    }
  }

  // send last packet (if pending)
  if (j != 0) BME280sCharacteristic.writeValue(ToSend,j);

  return(true);
}

/**
 * called when central connects
 */
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  SERIAL_PORT.print("Connected event, central: ");
  SERIAL_PORT.println(central.address());
}

/**
 * called when central disconnects
 */
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  SERIAL_PORT.print("Disconnected event, central: ");
  SERIAL_PORT.println(central.address());
  Interval = FIRST_SENDINTERVAL;      // reset
  uint8_t AltitudeInMeter = 1;
  uint8_t TempInCelsius = 1;
}

/////////////////////////////// Sleep routines /////////////////////////////////////////////////

/**
 * set peripheral to sleep
 */
void PeripheralSleep(){
  SERIAL_PORT.println("Time to go to sleep\n");
  
  // stop advertising or disconnect
  if(BLE.connected()){
    BLE.disconnect();
    while (BLE.connected()) BLE.poll();
  }
  else
    BLE.stopAdvertise();
    
  SPI.end();
  BLE.end();

  // Set timeout for wakeup
  SetRTC();

  goToSleep();
}

/**
 * Wakeup peripheral
 */
void PeripheralWakeup(){
  
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_MAX);
  
  // Go back to using the main clock
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_HFRC_3MHZ);
 
  // Enable Serial
#ifndef VERSION_1
  am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_UART0);
#endif
  SERIAL_PORT.begin(115200);
  while (!SERIAL_PORT) delay(100);
  
  // disable repeat
  rtc.setAlarmMode(0); 
  
#ifndef VERSION_1
    // enable BLEloop() timer
    am_hal_ctimer_start(7, AM_HAL_CTIMER_TIMERB);
    
    // enable SPI IOM
    if (Enable_SPI_IOM(MOSI, MISO) < 0)
      SERIAL_PORT.println("Could not enable IOM for SPI");
    
    // enable ADC
    powerControlADC(true);

    // add others if needed  !!!!!
    // e.g. wire (see example22)
#endif

  StartBME280();
  
  StartBLE();
}

#ifndef VERSION_1
/**
 * enable SPI IOM based on the MOSI and MISO. only needed for V2
 */
int Enable_SPI_IOM(int mosi, int miso) {
  int ret = -1;
  if     (mosi == 7  && miso == 6 ) {am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_IOM0); ret = 0;}    
  else if(mosi == 10 && miso == 9 ) {am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_IOM1); ret = 1;}  
  else if(mosi == 28 && miso == 25) {am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_IOM2); ret = 2;}    
  else if(mosi == 44 && miso == 40) {am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_IOM4); ret = 4;}    
  else if(mosi == 47 && miso == 49) {am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_IOM5); ret = 5;}
#if defined (AM_PACKAGE_BGA)
  else if(mosi == 38 && miso == 43) {am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_IOM3); ret = 3;}  
#endif

  return ret;
}
#endif  // VERSION_1

/**
 * set RTC to wake up on next time
 */
void SetRTC() {

  uint8_t m = 60 - DOWNTIME;

#ifdef VERSION_1    //!!! some jokemaker decided to move the order for SetTime and SetAlarm between V1 and V2  !!!
 // Manually set RTC date and time
  rtc.setTime(12, m, 0, 0, 25, 1, 23); // 12:m:00.00, Jan 25, 2023 (hh, mm, ss, hund, dd, mm, yy)

  // Set the RTC's alarm
  rtc.setAlarm(13, 0, 0, 0, 25, 1); // 13:00:00.00, Jan 25 (hh, mm, ss, hund, dd, mm). Note: No year alarm register
#else
  // Manually set RTC date and time
  rtc.setTime(0, 0, m, 12, 25, 1, 23); // 12:m:00.00, Jan 25, 2023 (hund, ss, mm, hh, dd, mm, yy)

  // Set the RTC's alarm
  rtc.setAlarm(0, 0, 0, 13, 25, 1); // 13:00:00.00, Jan 25 (hund, ss, mm, hh, dd, mm). Note: No year alarm register
#endif

  // Set the RTC alarm mode
  /*
    0: Alarm interrupt disabled
    1: Alarm match every year   (hundredths, seconds, minutes, hour, day, month)
    2: Alarm match every month  (hundredths, seconds, minutes, hours, day)
    3: Alarm match every week   (hundredths, seconds, minutes, hours, weekday)
    4: Alarm match every day    (hundredths, seconds, minute, hours)
    5: Alarm match every hour   (hundredths, seconds, minutes)
    6: Alarm match every minute (hundredths, seconds)
    7: Alarm match every second (hundredths)
  */
  rtc.setAlarmMode(5);  // Set the RTC alarm to match on hours rollover
  rtc.attachInterrupt(); // Attach RTC alarm interrupt
  //printDateTime();
}

/**
 * Print the RTC's current date and time 
 */
void printDateTime()
{
  rtc.getTime();
  char dateTimeBuffer[25];
  sprintf(dateTimeBuffer, "20%02d-%02d-%02d %02d:%02d:%02d.%03d",
          rtc.year, rtc.month, rtc.dayOfMonth,
          rtc.hour, rtc.minute, rtc.seconds, rtc.hundredths);
  SERIAL_PORT.println(dateTimeBuffer);
}

/**
 * Power down gracefully
 */
void goToSleep()
{
  // Disable UART
  SERIAL_PORT.end();

  // Disable ADC &
  #ifdef VERSION_1
    power_adc_disable();
  #else
    powerControlADC(false);
  #endif
  
  // Force the peripherals off
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM0);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM1);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM2);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM3);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM4);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM5);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART0);
  am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART1);

  // BLE mbed::lowpower.timer needs to be stopped
#ifndef VERSION_1
   am_hal_ctimer_stop(7, AM_HAL_CTIMER_TIMERB); 
#endif  
 
  // Disable all pads
  for (int x = 0 ; x < 50 ; x++) {
    
#ifndef VERSION_1
    if(x != MOSI && x != MISO && x != SCK)       // do NOT disable the SPI pins
#endif
      am_hal_gpio_pinconfig(x, g_AM_HAL_GPIO_DISABLE);
  }
  
  //Power down flash, SRAM, cache
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_CACHE);         // Turn off CACHE
#ifndef VERSION_1
  am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_FLASH_512K);    // keep 512K memory powered given the size of the binary
#else
  am_hal_pwrctrl_memory_deepsleep_retain(AM_HAL_PWRCTRL_MEM_SRAM_384K);        // Retain all SRAM (~0.6 uA)
#endif
  
  // Keep the 32kHz clock running for RTC
  am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
  am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ);

  am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP); // Sleep forever

  // And we're back!
  PeripheralWakeup();
}

// Interrupt handler for the RTC
extern "C" void am_rtc_isr(void)
{
  // Clear the RTC alarm interrupt
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);
}

/************************************************************************************
 *  Copyright (c) January 2023, version 1.0     Paul van Haastrecht
 *  - initial version for Artemis / Apollo3 boards, Micromod nRF52840 on an MM ATP board
 *
 *  =========================  Highlevel description ================================
 *
 *  This basic reading example sketch will connect to an SPS30 for getting data and
 *  display the available data. The data can also be obtained on Bluetooth (BLE) APP 
 *  and Example23_central_sps30.ino. You can select any I2C port. 
 *
 *  =========================  Additional software =================================
 *  SPS30 driver : https://github.com/paulvha/sps30
 *  
 *  
 *  =========================  Hardware connections =================================
 *
 *  //////////////////////////////////////////////////////////////////////////////////
 *  ## I2C I2C I2C  I2C I2C I2C  I2C I2C I2C  I2C I2C I2C  I2C I2C I2C  I2C I2C I2C ##
 *  //////////////////////////////////////////////////////////////////////////////////
 *  NOTE 1:
 *  Depending on the Wire / I2C buffer size we might not be able to read all the values.
 *  The buffer size needed is at least 60 while on many boards this is set to 32. The driver
 *  will determine the buffer size and if less than 64 only the MASS values are returned.
 *  You can manually edit the Wire.h of your board to increase (if you memory is larg enough)
 *  One can check the expected number of bytes with the I2C_expect() call as in this example
 *  see detail document.
 *
 *  NOTE 2:
 *  As documented in the datasheet, make sure to use external 10K pull-up resistor on
 *  both the SDA and SCL lines. Otherwise the communication with the sensor will fail random.
 *
 *  NOTE 3:
 *  For sleep, clean and wakeup the SPS30 requires firmware level 2.2 and up.
 *  
 *  Where is Pin 1?
 *  Looking at the back of the SPS30, pin 1 is on the LEFT, Pin 5 is on the RIGHT.
 *  ..........................................................
 *
 *  Successfully tested on Artemis / Apollo3 / MM with ATP
 *  The pull-up resistors should be to 3V3
 *  SPS30 pin      ATP board
 *  1 VCC -------- 5V                 3v3  
 *  2 SDA -------- SDA -------===------| (pull up resistors 10K each)
 *  3 SCL -------- SCL -------===------|
 *  4 Select ----- GND (select I2c)
 *  5 GND -------- GND
 **********************************************************************************
 * USAGE
 **********************************************************************************

  Use Example23_central_SPS30 to read complete SPS30 informatio form the example23 peripheral

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
      -Unknown Characteristic (UUID starting 19b10031)
      -Unknown Characteristic (UUID starting 19b10032)
      -Descriptors

    - click on the multiple-downward arrows next to unknown characteristic (UUID starting 19b10012)
     - it will now show properties NOTIFY,READ
     - Descriptors : Notifications enable

    - click on the up arrow next to unknown characteristic (UUID starting 19b10011)

    - enter value 06 + send : will show values of the SPS30 at unknown characteristic (UUID starting 19b10032)
      it will flash as the data is sent in mutiple packages and only the last package is shown like  (0x) 30, 30, "00")

 *  ================================ Disclaimer ======================================
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  ===================================================================================
 *
 *  NO support, delivered as is, have fun, good luck !!
 */

//////////////////////////////////////////////////////////////
// SPS30  defines
//////////////////////////////////////////////////////////////
#include "sps30.h"

/**
 * define communication channel to use for SPS30
 */
#define SP30_COMMS Wire

/**
 * Although the SPS30 (according to the datasheet) can handle 100K/s
 * based on feedback from a user he could only the SPS30 to work
 * for longer time stable at 50K.
 * If you want to test that remove comments from define line below
 */
//#define USE_50K_SPEED 1

/** 
 * define SPS30 driver debug
 * 0 : no messages
 * 1 : request sending and receiving
 * 2 : request sending and receiving + show protocol errors 
 */
#define DEBUG 0

//////////////////////////////////////////////////////////////
// Bluetooth defines
//////////////////////////////////////////////////////////////

// CHANGE ArduinoBLE_P TO ArduinoBLE WHEN USING VERSION 2.X OF SPARKFUN LIBRARY
#include <ArduinoBLE_P.h>

/**
 * Define here the structure for the data to exchange.
 * This structure must be defined the same on the central and peripheral
 */
struct data_to_exchange{
  // SPS30 data
  float   MassPM1;        // Mass Concentration PM1.0 [μg/m3]
  float   MassPM2;        // Mass Concentration PM2.5 [μg/m3]
  float   MassPM4;        // Mass Concentration PM4.0 [μg/m3]
  float   MassPM10;       // Mass Concentration PM10 [μg/m3]
  float   NumPM0;         // Number Concentration PM0.5 [#/cm3]
  float   NumPM1;         // Number Concentration PM1.0 [#/cm3]
  float   NumPM2;         // Number Concentration PM2.5 [#/cm3]
  float   NumPM4;         // Number Concentration PM4.0 [#/cm3]
  float   NumPM10;        // Number Concentration PM4.0 [#/cm3]
  float   PartSize;       // Typical Particle Size [μm]
  byte    Status;         // sleep (0), wakeup (1);
} p;

// To improve synchronization there are 2 checks MAGICNUM in byte 0 and length in byte 1
#define MAGICNUM 0XCF     // should be byte 0 in new message

// MAX Up to 29 characters for BLE name
const char BLE_PERIPHERAL_NAME[] = "Peripheral SPS30 BLE";

// create the SPS30 service
BLEService SPS30Service("19B10030-E8F2-537E-4F6C-D104768A1214");

// create characteristic and allow remote device to read and write (RECEIVE)
BLEByteCharacteristic SPS30rCharacteristic("19B10031-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

// create characteristic and allow remote device to get notifications (SEND max 100 bytes)
BLECharacteristic SPS30sCharacteristic("19B10032-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 100);

// enable HCI layer debug (remove comments)
//#define BLE_Debug

//////////////////////////////////////////////////////////////
// Program defines
//////////////////////////////////////////////////////////////

// enable sketch data (disable : comment out)
#define SKETCH_SHOW_DATA

// FIRST interval in seconds after connect
#define FIRST_SENDINTERVAL  10

// Interval in seconds between sending an update
#define SENDINTERVAL  30

// If the SPS30 was set to sleep and you reboot or reload the peripheral software
// it will not detect the SPS30 anymore, as it is still to sleep. 
// You then need to remove and re-apply the power to the SPS30.
//
// By setting WakeupOnDisconnect to true as the central disconnects AND the SPS30
// was set to sleep, the SPS30 will get a wakeup-call. Setting 'false' it will not do a wakeup.
bool WakeupOnDisconnect = true;

///////////////////////////////////////////////////////////////
/////////// NO CHANGES BEYOND THIS POINT NEEDED ///////////////
///////////////////////////////////////////////////////////////

uint16_t Interval;            // Interval time between sending auto-update
bool     EnableSend = true;   // start sending unless requested to stop
char     input[10];           // keyboard input buffer
int      inpcnt = 0;          // keyboard input buffer length
bool     SetSleep = false;    // in case SPS30 set to sleep = true
SPS30    sps30;               // create constructor

void setup() {

  Serial.begin(115200);
  delay(1000);
  serialTrigger((char *) "Example23: SPS30_ph_with BLE. press <enter> to start");

  //
  // SPS30 INIT
  //
  StartSPS30();
  
  //
  // BLE INIT
  //
  StartBLE();
}

void loop() {

  // This routine will trigger the HCI-layer flow
  // make sure this called often as part of your program loop
  //
  BLE.poll();
  
  //
  // handle any keyboard input
  //
  if (Serial.available()) {
    while (Serial.available()) handle_input(Serial.read());
  }

  //
  // handle anything received from Central
  //
  if (SPS30rCharacteristic.written()) {
    CentralRequestReceived();
  }

  //
  // if allowed to send SPS30 update and connected.
  //
  if (EnableSend && BLE.connected()) {

    // Interval for reading & sending SPS30
    if (--Interval == 0 ){
      CreateSendData();
      Interval = SENDINTERVAL;
    }
    else {
      // wait 10 x 100mS while checking / triggering BLE
      for (byte i = 0; i < 10; i++){
        BLE.poll();
        delay(100);
      } // wait
    } // interval has not timed out
  } // connected
}

/////////////////////////////////////// program functions //////////////////////////////////////

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
  Serial.println(F("1.  Request (re)START sending data\r"));
  Serial.println(F("2.  Request STOP sending data\r"));
  Serial.println(F("3.  Request SPS30 clean\r"));
  Serial.println(F("4.  Request SPS30 sleep\r"));
  Serial.println(F("5.  Request SPS30 wakeup\r"));
  Serial.println(F("6.  Request NOW sending data\r"));
}

/**
 * Act on command. either received from the local serial or received from the connected central
 */
enum key_input{
  REQUEST_START = 0x1,
  REQUEST_STOP,
  REQUEST_CLEAN,
  REQUEST_SLEEP,
  REQUEST_WAKEUP,
  REQUEST_NOW
};

/** 
 * could be entered on peripheral or central
 */
void handle_cmd(uint8_t req)
{
  switch (req)
  {
    case REQUEST_WAKEUP:
      Serial.print(F("Request wakeup\r\n"));
      SPS30_powerSet(true);
      break;

    case REQUEST_SLEEP:
      Serial.print(F("Request sleep\r\n"));
      SPS30_powerSet(false);
      break;

    case REQUEST_CLEAN:
      Serial.print(F("Request clean\r\n"));
      SPS30_cleanNow();
      break;

    case REQUEST_STOP:
      Serial.print(F("Stop sending\r\n"));
      EnableSend = false;
      break;

    case REQUEST_START:
      Serial.print(F("(re)Start sending\r\n"));
      EnableSend = true;
      Interval = 1;     // send at next turn
      break;

    case REQUEST_NOW:
      Serial.print(F("Sending SPS30 info now\r\n"));
      if (SetSleep) SPS30_powerSet(true);
      CreateSendData();
      Interval = SENDINTERVAL; // next one after SENDINTERVAL if enabled
      break;
        
    default:
      Serial.print("Unknown request ");
      Serial.println(req);
      break;
  }
}

/**
 *  @brief : continued loop after fatal error
 *  @param mess : message to display
 *  @param r : error code
 *
 *  if r is zero, it will only display the message
 */
void Errorloop(char *mess, uint8_t r)
{
  if (r) ErrtoMess(mess, r);
  else Serial.println(mess);
  Serial.println(F("Program on hold"));
  for(;;) delay(100000);
}

/**
 *  @brief : display error message
 *  @param mess : message to display
 *  @param r : error code
 *
 */
void ErrtoMess(char *mess, uint8_t r)
{
  char buf[80];

  Serial.print(mess);

  sps30.GetErrDescription(r, buf, 80);
  Serial.println(buf);
}

/**
 * serialTrigger prints repeated message, then waits for enter
 * to come in from the serial port.
 */
void serialTrigger(char * mess)
{
  Serial.println();

  while (!Serial.available()) {
    Serial.println(mess);
    delay(2000);
  }

  while (Serial.available())
    Serial.read();
}

/////////////////////////////////////// BLE functions ////////////////////////////////////////

/**
 * Start BLE
 */
void StartBLE()
{
  Serial.println("Starting BLE");

#ifdef BLE_Debug
  BLE.debug(Serial);         // enable display HCI commands on ArduinoBLE_P
#endif

  // begin initialization
  if (!BLE.begin()) {
    Errorloop((char *) "Could not start BLE.", 0);
  }

  // set the local name peripheral advertises
  BLE.setLocalName(BLE_PERIPHERAL_NAME);

  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(SPS30Service);

  // add the characteristics to the service
  SPS30Service.addCharacteristic(SPS30rCharacteristic);
  SPS30Service.addCharacteristic(SPS30sCharacteristic);

  // add the service
  BLE.addService(SPS30Service);

  SPS30rCharacteristic.writeValue(0);

  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // start advertising
  BLE.advertise();

  Interval = FIRST_SENDINTERVAL;      // reset

  Serial.println(F("Ready to go ...\r"));
  String address1 = BLE.address();
  Serial.print(F("local address "));  Serial.print(address1);
  Serial.print(F("\r\nlocal name '")); Serial.print(BLE_PERIPHERAL_NAME);
  Serial.print("'\r\n");
}

/**
 * This routine is called when data has been received from the host / central
 * we only expect 1 BYTE.. but have a buffer for 20.. who knows in the future
 */
void CentralRequestReceived()
{
  int len = SPS30rCharacteristic.valueLength();
  uint8_t buf[20];

  if (len > 20) len = 20;

  // obtain received value(s)
  SPS30rCharacteristic.readValue(buf, len);

/** set to 1 to enable debug */
#if 0
  Serial.print(F("Received command with length: "));
  Serial.print(len);
  Serial.print("\r\n");

  for (int i = 0 ; i< len ; i++) {
    Serial.print(" 0x");
    Serial.print(buf[i], HEX);
  }
  Serial.println();
#endif

  // only ONE byte is expected for this solution
  handle_cmd(buf[0] - '0');  // ascii to hex
}

/**
 * sent a data message to the central/host
 *
 * @param buf : data to be sent
 * @param len : length of data
 *
 * As we send over notify the max length is the MTU-length (often the default 23)
 * 3 bytes are used for handles so we keep it to max 20
 * Of course we have to do the opposite on the central side to combine
 */
bool SendReplyCentral(uint8_t *buf, uint8_t len)
{
  uint8_t ToSend[25];
  int i, j;             // conversion counter

  if (!BLE.connected()) {
    Serial.print(F("Error: Not connected (anymore)\r\n"));
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
      SPS30sCharacteristic.writeValue(ToSend, j);
      j=0;
    }
  }

  // send last packet (if pending)
  if (j != 0) SPS30sCharacteristic.writeValue(ToSend,j);

  return(true);
}

/**
 * called when central connects
 */
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

/**
 * called when central disconnects
 */
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  if (SetSleep && WakeupOnDisconnect) SPS30_powerSet(true); 
  Interval = FIRST_SENDINTERVAL;      // reset
}

/////////////////////////////////////// SPS30 functions ////////////////////////////////////////

/**
 * start SPS30
 */
void StartSPS30()
{  
  Serial.println(F("Trying to find SPS30."));

  // set driver debug level
  sps30.EnableDebugging(DEBUG);

  // Begin communication channel
  SP30_COMMS.begin();

  if (sps30.begin(&SP30_COMMS) == false) {
    Errorloop((char *) "Could not set I2C communication channel.", 0);
  }
  
  // check for SPS30 connection
  if (! sps30.probe()){
    Serial.println("Maybe SPS30 was set to sleep in earlier session");
    Serial.println("Remove and reconnect SPS30 power");
    Errorloop((char *) "could not probe / connect with SPS30.", 0);
  }
  else  Serial.println(F("Detected SPS30."));

  // reset SPS30 connection
  if (! sps30.reset()) Errorloop((char *) "could not reset.", 0);

#ifdef SKETCH_SHOW_DATA
  // read device info
  GetDeviceInfo();
#endif

  // start measurement
  if (sps30.start()) Serial.println(F("Measurement started"));
  else Errorloop((char *) "Could NOT start measurement", 0);
  
  if (sps30.I2C_expect() == 4){
    Serial.println(F(" !!! Due to I2C buffersize only the SPS30 MASS concentration is available !!! \n"));
  }
}

/**
 * perform a clean on SPS30
 */
void SPS30_cleanNow()
{
  // clean now
  if (sps30.clean() == true)
    Serial.println(F("fan-cleaning manually started"));
  else
    Serial.println(F("Could NOT manually start fan-cleaning"));
}

/**
 * set SPS30 to sleep or wakeup.
 * 
 * @param setWakeup
 *  true : wakeup
 *  false : sleep
 */
void SPS30_powerSet(bool setWakeup)
{
  int ret;

  if (setWakeup){

    // wakeup SPS30
    ret = sps30.wakeup();

    if (ret != SPS30_ERR_OK) {
      ErrtoMess((char *) "ERROR: Could not wakeup SPS30. ", ret);
    }
    
    SetSleep = false;
    p.Status = 1;
  }
  
  else {

    // put the SPS30 to sleep
    ret = sps30.sleep();

    if (ret != SPS30_ERR_OK) {
      ErrtoMess((char *) "ERROR: Could not set SPS30 to sleep. ", ret);
    }
    
    SetSleep = true;
    p.Status = 0;
  }
}


#ifdef SKETCH_SHOW_DATA
/**
 * @brief : read and display device info
 */
void GetDeviceInfo()
{
  char buf[32];
  uint8_t ret;
  SPS30_version v;

  //try to read serial number
  ret = sps30.GetSerialNumber(buf, 32);
  if (ret == SPS30_ERR_OK) {
    Serial.print(F("Serial number : "));
    if(strlen(buf) > 0)  Serial.println(buf);
    else Serial.println(F("not available"));
  }
  else
    ErrtoMess((char *) "could not get serial number. ", ret);

  // try to get product name
  ret = sps30.GetProductName(buf, 32);
  if (ret == SPS30_ERR_OK)  {
    Serial.print(F("Product name  : "));

    if(strlen(buf) > 0)  Serial.println(buf);
    else Serial.println(F("not available"));
  }
  else
    ErrtoMess((char *) "could not get product name. ", ret);

  // try to get version info
  ret = sps30.GetVersion(&v);
  if (ret != SPS30_ERR_OK) {
    Serial.println(F("Can not read version info."));
    return;
  }

  Serial.print(F("Firmware level: "));   Serial.print(v.major);
  Serial.print(".");  Serial.println(v.minor);

  Serial.print(F("Library level : "));  Serial.print(v.DRV_major);
  Serial.print(".");  Serial.println(v.DRV_minor);
}
#endif //SKETCH_SHOW_DATA

/**
 * @brief : read and display all values
 */
bool CreateSendData()
{
  uint8_t ret, error_cnt = 0;
  struct sps_values val;

  // if SPS30 is sleeping 
  if (SetSleep) {    
    p.Status = 0;
    SendReplyCentral((uint8_t *) &p, sizeof(struct data_to_exchange));
    return(true);
  }
  
  // loop to get data
  do {

#ifdef USE_50K_SPEED                // see top of sketch
    SP30_COMMS.setClock(50000);     // set to 50K
    ret = sps30.GetValues(&val);
    SP30_COMMS.setClock(100000);    // reset to 100K in case other sensors are on the same I2C-channel
#else
    ret = sps30.GetValues(&val);
#endif

    // data might not have been ready
    if (ret == SPS30_ERR_DATALENGTH){

        if (error_cnt++ > 3) {
          ErrtoMess((char *) "Error during reading values: ",ret);
          return(false);
        }
        delay(1000);
    }

    // if other error
    else if(ret != SPS30_ERR_OK) {
      ErrtoMess((char *) "Error during reading values: ",ret);
      return(false);
    }

  } while (ret != SPS30_ERR_OK);

  p.MassPM1  = val.MassPM1;
  p.MassPM2  = val.MassPM2;
  p.MassPM4  = val.MassPM4;
  p.MassPM10 = val.MassPM10;
  p.NumPM1   = val.NumPM1;
  p.NumPM2   = val.NumPM2;
  p.NumPM4   = val.NumPM4;
  p.NumPM10  = val.NumPM10;
  p.PartSize = val.PartSize;
  p.Status   = 1;             //not sleeping
  
#ifdef SKETCH_SHOW_DATA

  Serial.println(F("-------------Mass -----------    ------------- Number --------------   -Average-"));
  Serial.println(F("     Concentration [μg/m3]             Concentration [#/cm3]             [μm]"));
  Serial.println(F("P1.0\tP2.5\tP4.0\tP10\tP0.5\tP1.0\tP2.5\tP4.0\tP10\tPartSize\n"));

  Serial.print(val.MassPM1);
  Serial.print(F("\t"));
  Serial.print(val.MassPM2);
  Serial.print(F("\t"));
  Serial.print(val.MassPM4);
  Serial.print(F("\t"));
  Serial.print(val.MassPM10);
  Serial.print(F("\t"));
  Serial.print(val.NumPM0);
  Serial.print(F("\t"));
  Serial.print(val.NumPM1);
  Serial.print(F("\t"));
  Serial.print(val.NumPM2);
  Serial.print(F("\t"));
  Serial.print(val.NumPM4);
  Serial.print(F("\t"));
  Serial.print(val.NumPM10);
  Serial.print(F("\t"));
  Serial.print(val.PartSize);
  Serial.print(F("\n"));
#endif //SKETCH_SHOW_DATA

  SendReplyCentral((uint8_t *) &p, sizeof(struct data_to_exchange));
  return(true);
}

/*
  Arduino central <=>  peripheral comm.  

  This example demonstrates a basic BLE central functionality.
  It is mend to show how the example10_ph_BME280 peripheral could be addressed and how to 
  to sent and/or request data 
  
*********************************************************************
* DEPENDENCIES
*********************************************************************
  It will require a good working function of ArduinoBLE. 
  For Sparkfun Apollo3 library 1.2.1 a special patched ArduinoBLe (lower case e) and 
  ExactLE package needs to be installed

  You might get a warning
  Error while detecting libraries included by /home/paul/Arduino/libraries/ArduinoBLe/src/utility/HCIExactleTransport.cpp
  next that.. it is a bug in Arduino IDE
*********************************************************************
* USAGE
*********************************************************************
  How to use this example:

  Make sure to load on another Apollo3 board the example10_ph_BME280.ino sketch.
  
  Start the peripheral: example10_ph_BME280
  
  Start this central

  Send a menu option or just wait
  
  Version 1.0 / February 2021 / paulvha
  -initial version
*/

/***********************************************************************/
// Define here the structure for the data to exchange.
// This structure must be defined the same on the central and periperal
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

//*********************************************************************
//*  NO CHANGES NEEDED BEYOND THIS POINT
//*********************************************************************
#include "bridge.h"

// receive variables
uint8_t receiveBuf[MAXRECEIVEBUF];// hand off data buffer from Bluetooth to user level
uint16_t ReceiveBufLen;           // length of data received in buffer
bool RespReceive = false;         // indicator new data in buffer
uint8_t StructBuf[sizeof(struct data_to_exchange)];

void setup() {

  SERIAL_PORT.begin(115200);
  while(! SERIAL_PORT);
  SERIAL_PORT.println(F("Example10: Arduino Central BME280.\r"));

  //
  // start BLE
  //
  Start_BLE();
  
  //
  // display options
  //
  display_menu();
}

void loop() {

  // poll and check connect (in bridge.cpp)
  if (!CheckConnect()) {
    SERIAL_PORT.println(F("Disconnected\r"));
    Start_BLE();
  }

  // handle any new data received over Bluetooth peripheral
  if (RespReceive) {
    HandleResp();
    display_menu();
  }

  // handle any keyboard input
  if (SERIAL_PORT.available()) {
    while (SERIAL_PORT.available()) handle_input(SERIAL_PORT.read());
  }
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

//***************************************************************
//*
//* handle keyboard input
//*
//***************************************************************
char input[10];                   // keyboard input buffer
int inpcnt = 0;                   // keyboard input buffer length

void handle_input(char c)
{
  uint8_t buf[1];
  
  if (c == '\r') return;          // skip CR
        
  if (c != '\n') {                // act on linefeed
    input[inpcnt++] = c;
    if (inpcnt < 9) return; 
  }
  
  input[inpcnt] = 0x0;
  buf[0] = (uint8_t) atoi(input);

  switch (buf[0])
  {
    case REQUEST_METERS:
    case REQUEST_FEET:
    case REQUEST_CELSIUS:
    case REQUEST_FRHEIT:
    case REQUEST_STOP:
    case REQUEST_START:
    case REQUEST_NOW:
        SERIAL_PORT.print(F("Sending request\r\n"));
        SendReplyPeripheral(buf, 1);
        break;
    
    default:
        SERIAL_PORT.print(F("Unknown request: 0x"));
        SERIAL_PORT.println(buf[0], HEX);
        break;
  }
  
  display_menu();
  
  // reset keyboard buffer
  inpcnt = 0;
}

//*****************************************************************************
//
// Sent request to peripheral
//
//*****************************************************************************
void SendReplyPeripheral(uint8_t *buf, uint16_t len)
{

#ifdef BLE_SHOW_DATA
  for(uint16_t i = 0; i < len; i++){ 
    Serial.print(" 0x");
    Serial.print(buf[i], HEX);
  }
  Serial.println(" ... done");
#endif 

  if (! SendData(buf, len) ){
    SERIAL_PORT.print(F("Error during sending data\r\n"));
  }
}

//*****************************************************************************
//
// Hand-off the responds received from the peripheral
// 
// Hand-off the data and let the "stack" clear with all the returns pending
// to ArduinoBLE functions to prevent stack-overrun. 
// The display of the data is triggered once returned in loop()
// 
// The total message can be split across multiple packets.
// The first byte in a the first package of a new messages will indicate the
// total length of the message.
//*****************************************************************************
void HandlePeripheralResp(const uint8_t *buf, uint16_t len)
{
  static uint16_t RespOffset = 0;
  uint8_t i;
  
  // If start of a brand new message, it might be split across different packets
  if (RespOffset == 0) {
    ReceiveBufLen = buf[0];    // first byte of new message is length
    i = 1;                     // offset to start copy data
    
#ifdef BLE_SHOW_DATA 
    SERIAL_PORT.print(F("Total Data length in this message = "));
    SERIAL_PORT.print(ReceiveBufLen);
    SERIAL_PORT.print("\r\n");
#endif
   
  }
  else
    i = 0;                    // offset to start copy data
  
#ifdef BLE_SHOW_DATA 
  SERIAL_PORT.print(F("Current packet len = "));
  SERIAL_PORT.print(len);
  SERIAL_PORT.print("\r\n");
#endif

  // copy data of this packet in the receive buffer
  for (; i < len ; i++) {
    
    // prevent buffer overrun
    if (RespOffset < MAXRECEIVEBUF)
        receiveBuf[RespOffset++] = buf[i];
  }
  
  // did we receive the complete message. +1 because of length byte at start
  if (RespOffset + 1> ReceiveBufLen) {
    RespReceive = true;       // indicate message received
    RespOffset = 0;           // reset for next message
  }
}

//*****************************************************************************
//
// Handle / display responds received from the peripheral
// as the data on an String-characteristic we can NOT have a 0x0 in the data
// as such all is sent as ascii and we need to transform that back
//
//*****************************************************************************
void HandleResp()
{
#ifdef BLE_SHOW_DATA 
  SERIAL_PORT.println(F("Raw data:"));
  for(uint8_t i = 0 ; i < ReceiveBufLen ; i++) {
    SERIAL_PORT.print(F(" 0x"));
    SERIAL_PORT.print(receiveBuf[i], HEX);
  }
  SERIAL_PORT.println();
#endif

  int i, j;
  
  // change ascii back to uint8_t
  for ( i = 0,  j = 0; i < ReceiveBufLen; i++,j++){

    if (receiveBuf[i] > 0x39) StructBuf[j] = receiveBuf[i] - 0x37;
    else StructBuf[j] = receiveBuf[i] - 0x30;

    StructBuf[j] = StructBuf[j] << 4;
    i++;
       
    if (receiveBuf[i] > 0x39) StructBuf[j] |= receiveBuf[i] - 0x37;
    else StructBuf[j] |= receiveBuf[i] - 0x30;
    
    //SERIAL_PORT.println(StructBuf[j], HEX);
  }
  
  display_BME280(StructBuf, sizeof(struct data_to_exchange));

  RespReceive = false;
}

//*****************************************************************************
//
// display data
//
//*****************************************************************************
void display_BME280(uint8_t *buf, uint16_t len)
{
  struct data_to_exchange *p = (data_to_exchange *) buf;
  
  // temperature
  SERIAL_PORT.print(F("\r\nTemperature\t"));
  SERIAL_PORT.print(p->temperature);
  if (p->celsius)  SERIAL_PORT.println("*C\r");
  else  SERIAL_PORT.println(F("*F\r"));

  // humidity
  SERIAL_PORT.print(F("Humidity\t"));
  SERIAL_PORT.print(p->humidity);
  SERIAL_PORT.println("%\r");

   // pressure
  SERIAL_PORT.print(F("Pressure\t"));
  SERIAL_PORT.print(p->pressure);
  SERIAL_PORT.println(F(" hPa\r"));

  // altitude
  SERIAL_PORT.print(F("Altitude\t"));
  SERIAL_PORT.print(p->altitude);
  if (p->meter) SERIAL_PORT.println(F(" Meter\r\n"));
  else SERIAL_PORT.println(F(" Feet\r\n"));
}
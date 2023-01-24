/*
  Arduino central <=>  peripheral comm.

  This example demonstrates a basic BLE central functionality.
  It is mend to show how the example23_ph_SPS30 peripheral could be accessed and how
  to sent and/or request data

*********************************************************************
* DEPENDENCIES
*********************************************************************
  It will require a good working function of ArduinoBLE.
  For Sparkfun Apollo3 library V1.2.3 a special patched ArduinoBLE_P .
  On Sparkfun library V1.2.3. The HCI package needs to be installed

  You might get a warning
  Error while detecting libraries included by /home/paul/Arduino/libraries/ArduinoBLe/src/utility/HCIExactleTransport.cpp
  ignore that.. it is a bug in Arduino IDE
*********************************************************************
* USAGE
*********************************************************************
  How to use this example:

  Make sure to load on another board the example23_ph_SPS30 sketch.

  Start the peripheral

  Start this central

  Send a menu option or just wait

*********************************************************************
* VERSIONING
*********************************************************************
  Version 1.0 / January 2023 / paulvha
  -initial version
*/

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
  byte    Status;         // sps30 is sleep (0) or wakeup (1);
} p;

// To improve synchronization there are 2 checks:
#define MAGICNUM 0XCF      // should be byte 0 in new message
#define MAGICLEN sizeof(p) // should be byte 1 (length) in new message

//*********************************************************************
//*  NO CHANGES NEEDED BEYOND THIS POINT
//*********************************************************************
#include "bridge.h"

// receive variables
uint8_t receiveBuf[MAXRECEIVEBUF];// hand off data buffer from Bluetooth to user level
uint16_t ReceiveBufLen;           // length of data received in buffer
bool RespReceive = false;         // indicator new data in buffer

void setup() {

  SERIAL_PORT.begin(115200);
  while(! SERIAL_PORT);
  SERIAL_PORT.println(F("Example23: Arduino Central SPS30.\r"));

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
  
  //
  // poll and check connect (in bridge.cpp)
  //
  if (!CheckConnect()) {
    SERIAL_PORT.println(F("Disconnected\r"));
    stopconnect(false);
    Start_BLE();
  }

  //
  // handle any new data received over Bluetooth peripheral
  //
  if (RespReceive) {
    HandleResp();
    display_menu();
  }
  
  //
  // handle any keyboard input
  //
  if (SERIAL_PORT.available()) {
    while (SERIAL_PORT.available()) handle_input(SERIAL_PORT.read());
  }
}

void display_menu()
{
  SERIAL_PORT.println(F("1.  Request (re)START sending data\r"));
  SERIAL_PORT.println(F("2.  Request STOP sending data\r"));
  SERIAL_PORT.println(F("3.  Request SPS30 clean\r"));
  SERIAL_PORT.println(F("4.  Request SPS30 sleep\r"));
  SERIAL_PORT.println(F("5.  Request SPS30 wakeup\r"));
  SERIAL_PORT.println(F("6.  Request NOW sending data\r"));
  SERIAL_PORT.println(F("7.  Disconnect now\r"));
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
  REQUEST_NOW,
  DISCONNECT_NOW
  
};

/**
 * handle keyboard input
 */
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
    case REQUEST_CLEAN:
    case REQUEST_SLEEP:
    case REQUEST_WAKEUP:
    case REQUEST_STOP:
    case REQUEST_START:
    case REQUEST_NOW:
        SERIAL_PORT.print(F("Sending request\r\n"));
        buf[0] = buf[0] + '0';  // send as ascii
        SendReplyPeripheral(buf, 1);
        break;
    case DISCONNECT_NOW:
        SERIAL_PORT.println(F("Disconnect request\r"));
        stopconnect(true);
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

/**
 * wait for enter
 */
void flush_wait() {
  // clear any pending
  while (SERIAL_PORT.available()) {
    SERIAL_PORT.read();
    delay(100);
  }

  // wait for enter
  while (! SERIAL_PORT.available());

  // clear buffer
  while (SERIAL_PORT.available()) {
    SERIAL_PORT.read();
    delay(100);
  }
}

/**
 * called when locally disconnect performed
 * will wait for <enter> before reconnecting
 */
void stopconnect(bool dis)
{
  if (dis) BLE.disconnect();

  SERIAL_PORT.println(F("Press <enter> to reconnect\r"));
  flush_wait();
}

/**
 * Sent request to peripheral
 */
void SendReplyPeripheral(uint8_t *buf, uint16_t len)
{

#ifdef BLE_SHOW_DATA
  for(uint16_t i = 0; i < len; i++){
    SERIAL_PORT.print(" 0x");
    SERIAL_PORT.print(buf[i], HEX);
  }
  SERIAL_PORT.println(" ... done");
#endif

  if (! SendData(buf, len) ){
    SERIAL_PORT.print(F("Error during sending data\r\n"));
  }
}

/**
 *
 * Hand-off the responds received from the peripheral
 *
 * Hand-off the data and let the "stack" clear with all the returns pending
 * to ArduinoBLE functions to prevent stack-overrun.
 * The display of the data is triggered once returned in loop()
 *
 * The total message can be split across multiple packets.
 * The first byte in a the first package of a new messages will indicate the
 * total length of the message.
 */
void HandlePeripheralResp(const uint8_t *buf, uint16_t len)
{
  static uint16_t RespOffset = 0;
  uint8_t i = 0;                // offset to start copy data

  // If start of a brand new message, it might be split across different packets
  if (RespOffset == 0) {
    // check for correct packet start
    if (buf[0] != MAGICNUM || buf[1] != MAGICLEN ) return;

    ReceiveBufLen = buf[1];    // second byte of new message is length
    i = 2;                     // offset to start copy data

#ifdef BLE_SHOW_DATA
    SERIAL_PORT.print(F("Total Data length in this message = "));
    SERIAL_PORT.print(ReceiveBufLen);
    SERIAL_PORT.print("\r\n");
  }

  SERIAL_PORT.print(F("Current packet len = "));
  SERIAL_PORT.print(len);
  SERIAL_PORT.print("\r\n");
#else
  }
#endif

  // copy data of this packet in the receive buffer
  for (; i < len ; i++) {

    // prevent buffer overrun
    if (RespOffset < MAXRECEIVEBUF)
        receiveBuf[RespOffset++] = buf[i];
  }

  // did we receive the complete message. +1 because of length byte at start
  if (RespOffset + 1 > ReceiveBufLen) {
    RespReceive = true;       // indicate message received
    RespOffset = 0;           // reset for next message
  }
}

/**
 * Handle / display responds received from the peripheral
 */
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

  display_SPS30(receiveBuf, sizeof(struct data_to_exchange));

  RespReceive = false;
}

/**
 * display data
 */
void display_SPS30(uint8_t *buf, uint16_t len)
{
  struct data_to_exchange *p = (data_to_exchange *) buf;

  if (p->Status == 0) {
    SERIAL_PORT.println(F("SPS30 is set to Sleep"));
    return;
  }

  SERIAL_PORT.println(F("-------------Mass -----------    ------------- Number --------------   -Average-"));
  SERIAL_PORT.println(F("     Concentration [μg/m3]             Concentration [#/cm3]             [μm]"));
  SERIAL_PORT.println(F("P1.0\tP2.5\tP4.0\tP10\tP0.5\tP1.0\tP2.5\tP4.0\tP10\tPartSize\n"));

  SERIAL_PORT.print(p->MassPM1);
  SERIAL_PORT.print(F("\t"));
  SERIAL_PORT.print(p->MassPM2);
  SERIAL_PORT.print(F("\t"));
  SERIAL_PORT.print(p->MassPM4);
  SERIAL_PORT.print(F("\t"));
  SERIAL_PORT.print(p->MassPM10);
  SERIAL_PORT.print(F("\t"));
  SERIAL_PORT.print(p->NumPM0);
  SERIAL_PORT.print(F("\t"));
  SERIAL_PORT.print(p->NumPM1);
  SERIAL_PORT.print(F("\t"));
  SERIAL_PORT.print(p->NumPM2);
  SERIAL_PORT.print(F("\t"));
  SERIAL_PORT.print(p->NumPM4);
  SERIAL_PORT.print(F("\t"));
  SERIAL_PORT.print(p->NumPM10);
  SERIAL_PORT.print(F("\t"));
  SERIAL_PORT.print(p->PartSize);
  SERIAL_PORT.print(F("\n"));
}

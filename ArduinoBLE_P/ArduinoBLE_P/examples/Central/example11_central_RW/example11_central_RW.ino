
// CHANGE ArduinoBLE_P TO ArduinoBLE WHEN USING VERSION 2.X OF SPARKFUN LIBRARY

#include <ArduinoBLE_P.h>
/*
  This sketch demostrates the sending and receiving of data between 2 BLE devices
  It acts as the central for the sketch example11_ph_RW.

  It will search the pheripheral based on the service-name, when detected it will connect, extract the
  attributes and checks for the 4 characteristics to be available.

  There are 2 characteristics (one for reading, one for writing) for binary data exchange and
  2 characteristics (one for reading, one for writing) for ASCII string exchange.

  In each message received the first byte MUST be the MAGICNUM.

  There are 3 tabs :
  RW_central_example11 : here is the user level data exchange performed
  BLE_Comm : All BLE related activities are in this tab.
  defines.h : Defines that are common for the tabs.


!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  PROBLEM / ERROR

  In first instance I used a single characteristic for String. This was set for BLERead and BLEWrite.
  When this sketch is performs a writeValue() it will set a message with length of 33. As the central
  is reading, the ATT layer of ArduinoBLE will check for a valueLength(). If available it will try to
  obtain that length (33) with readValue().

  Now as the remote / central is performing a writeValue() of a 5 character long value on the same
  characteristic this will be stored. The String_written_callback is done. The valuelength is now 5
  and we can obtain that value.

  HOWEVER....
  Now the central is performing another read, the ATT layer of ArduinoBLE will check for a valueLength().
  , which is NOW 5 !! It will try to obtain that length with readValue(). But from readValue() the
  String_read_callback is called and it will set a value of 33 characters. BUT only 5 will be read
  now.

  Of course we can, after reading what was send, immediately do a write of 33 character string, but
  actually the values from the pheripheral and central should be stored separatly in ArduinoBLE.

  SO YOU CAN ONLY USE BLEREAD AND BLEWRITE ON THE SAME CHARACTERISTIC IF THE LENGTH OF THE VALUES SEND
  BY EITHER PHERIPHERAL OR CENTRAL ARE THE SAME !!

  Actually the values from the pheripheral and central should be stored separatly in ArduinoBLE.

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  OBSERVATION 1
  Of course we can, after reading what was send, immediately do a write of 33 character string.
  You can set a dummy of (in this case) 33 characters and wait for BLERead_call_back to update that
  to the most actual value that you really want to send. That will only work if that latest value is
  of the same size of the dummy (33 characters in our case).

  OBSERVATION 2
  In any case where a remote is trying to read a characteristic, you MUST first set a value. in case of
  using BLERead_call_back the size of that value MUST be the same size of the next value.
  (see the description above for explanation).

  OBSERVATION 3
  You can use notify, but that will only send as bytes that fit in the agreed MTU size between peripheral
  and central after connect. The default size is 23. unfortunatly there is no library call to obtain
  the agreed MTU size.

  OBSERVATION 4
  A way around is to first set a new value with the correct size on the characteristic.
  Then use a NOTIFY characteristic to send SHORT message the remote to indicate that a
  read characteristic is ready to be read.

*/
# include "defines.h"

// receive variables
#define MAGICNUM 0XCF             // should be byte 0 in new message
#define MAXRECEIVEBUF   100       // size of receive buffer
uint8_t receiveBuf[MAXRECEIVEBUF];// hand off data buffer from Bluetooth to user level
uint16_t ReceiveBufLen;           // length of data received in buffer

// send variables
String  TestString ="ABCDEFGHIJKLMNOPQRSTUVW0123456789";
uint8_t TestBinbuf[]={MAGICNUM, 0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x50};
int8_t  t_offset = 0;             // needed for rolling string

void setup() {
   Serial.begin(115200);
   delay(1000);
   Serial.print("\nExampe11 central RW. Compiled on: ");
   Serial.println(__TIME__);

  // BLE init
  Start_BLE();                    // in BLE_Comm

  display_menu();

  Serial.println(F("ready to go"));
}

void loop() {

  // poll and check connect (in BLE_Comm)
  if (!CheckConnect()) {
    SERIAL_PORT.println(F("Disconnected\r"));
    Start_BLE();
  }

  // handle any keyboard input
  if (SERIAL_PORT.available()) {
    while (SERIAL_PORT.available()) handle_input(SERIAL_PORT.read());
  }
}

/**
 * handle keyboard input
 */
char input[10];                   // keyboard input buffer
int inpcnt = 0;                   // keyboard input buffer length

void handle_input(char c)
{
  /* this allows to enter more than one character (up to 9)
   * and then have more menu options. Here we only use the
   * first character.
   */
  if (c == '\r') return;          // skip CR

  if (c != '\n') {                // act on linefeed
    input[inpcnt++] = c;
    if (inpcnt < 9) return;
  }

  input[inpcnt] = 0x0;

  // we only use the first character here
  switch (input[0])
  {
    case '1':
        SERIAL_PORT.print(F("Receiving String\r\n"));
        RcvData(false);
        break;

    case '2':
        SERIAL_PORT.print(F("Receiving Binary\r\n"));
        RcvData(true);
        break;

    case '3':
        SERIAL_PORT.print(F("Sending String\r\n"));
        Send_data();
        break;

    case '4':
        SERIAL_PORT.print(F("Sending Binary\r\n"));
        TXData(TestBinbuf, sizeof(TestBinbuf), true);
        break;

    case '5':
        SERIAL_PORT.println(F("Disconnect request\r"));
        BLE.disconnect();
        WaitToReconnect();
        break;

    default:
        SERIAL_PORT.print(F("Unknown request: "));
        SERIAL_PORT.println(input[0]);
        break;
  }

  display_menu();

  // reset keyboard buffer
  inpcnt = 0;
}

/**
 * send a sliding string
 */
void Send_data(){
  int i,j;
  String t_send;

  j=t_offset++;

  if (t_offset == TestString.length()) t_offset=0;

  /* if the message is longer than 100 characters, you
   * could break it in blocks. The second byte of the
   * first message (after the MAGICNUM) could be the
   * total length of all the blocks.
   */
  // add Magic to indicate a valid start
  t_send += (char) MAGICNUM;

  // for each letter
  for (i = 0; i < TestString.length();i++){

   // copy from start offset
   t_send += TestString[j++];

   // loop around
   if (j == TestString.length()) j = 0;
  }

  Serial.print("Sending content: ");
  Serial.println(t_send.substring(1,t_send.length())); // skip magic
  Serial.print("length: ");
  Serial.println(t_send.length()-1);

  TXData((uint8_t *) t_send.c_str(), t_send.length(), false);
}

/**
 * Read data from a characteristic
 * @param bin : true = Binary characteristic, false is String characteristic
 */
void RcvData(bool bin)
{
  uint8_t TempBuf[MAXRECEIVEBUF];
  int lenr = MAXRECEIVEBUF;

  lenr = RXData(TempBuf, lenr, bin);

  if (! lenr) {
    Serial.println(F("Nothing received"));
    return;
  }

  /* if the message is longer than 100 characters, you
   * could break it in blocks. The second byte of the
   * first message (after the MAGICNUM) could be the
   * total length of all the blocks. you can then use
   * it to combine that in the receiveBuf untill the total
   * size has been received.
   */

  // check for correct packet start
  if (TempBuf[0] != MAGICNUM) {
    Serial.print(F("Invalid magic number : 0x"));
    Serial.println(TempBuf[0],HEX);
    return;
  }

  SERIAL_PORT.print(F("Total Data length in this message = "));
  SERIAL_PORT.print(lenr-1);
  SERIAL_PORT.print("\r\n");

  ReceiveBufLen = 0;

  // copy data of this packet in the receive buffer
  // skip magicnum
  for (int i = 1 ; i < lenr ; i++) {

    // prevent buffer overrun
    if (ReceiveBufLen < MAXRECEIVEBUF) {

      if (bin) Serial.print(TempBuf[i],HEX);
      else Serial.print((char) TempBuf[i]);
      /* this copy is not really necessary, but that way
       * you can use the data later on if you want to
       */
      receiveBuf[ReceiveBufLen++] = TempBuf[i];
    }
  }

  Serial.println();
}

void display_menu()
{
  SERIAL_PORT.println(F("1  Read a string from pheripheral"));
  SERIAL_PORT.println(F("2  Read a Binary buffer from pheripheral"));
  SERIAL_PORT.println(F("3  Send a ASCII string to pheripheral"));
  SERIAL_PORT.println(F("4  Send a Binary buffer to pheripheral"));
  SERIAL_PORT.println(F("5. Disconnect from peripheral"));
}

/**
 * called when disconnect  was performed
 * will wait for <enter> before reconnecting
 */
void WaitToReconnect()
{

  // clear any pending
  while (SERIAL_PORT.available()) {
    SERIAL_PORT.read();
    delay(100);
  }

  SERIAL_PORT.println(F("Press <enter> to reconnect\r"));

  // wait for enter
  while (! SERIAL_PORT.available());

  // clear buffer
  while (SERIAL_PORT.available()) {
    SERIAL_PORT.read();
    delay(100);
  }
}

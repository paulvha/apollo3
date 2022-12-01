// CHANGE ArduinoBLE_P TO ArduinoBLE WHEN USING VERSION 2.X OF SPARKFUN LIBRARY
// HOWEVER... if the blocksize is larger than the MTU... it only works on ArduinoBLE_P (p = patched)

#include <ArduinoBLE_P.h>
/*
  This sketch demostrates the sending and receiving of data between 2 BLE devices
  It acts as the central for the sketch example12_ph_RW_Notify.

  It will search the peripheral based on the service-name, when detected it will connect, extract the 
  attributes and checks for the 3 characteristics to be available.

  In each message received the first byte MUST be the MAGICNUM.  

  There are 4 tabs :
  example12_central_RW_Notify : here is the user level data exchange performed
  BLE_Comm  : All BLE related activities are in this tab. 
  defines.h : Defines that are common for the tabs.
  crc       : calculate the CRC for each block
  
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
  Challenge PROBLEM / ERROR
  
  When BLERead call back has been set we can get undesired behaviour.

  When a central wants to read a characteristic, ArduinoBLE will first check for valuelength. 
  If not zero it will perform readValue(). If BLERead call back was set, it will be called BEFORE
  copying the characteristic value to be send. The callback can now write a different value and that value 
  might have gotten a different length. Still ArduinoBLE will only copy the initial obtained valuelength.

  The issues :
  1. There MUST be a valuelength and thus a writeValue() must have been done. Otherwise the valuelength is
  zero and readValue() is never called.

  2. If readValue() is called the earlier obtained valuelength might be incorrect for the updated value. (done
  in the callback) 

  WorkArounds
  1) If your value is a fixed length all the time, on connect the peripheral can set a dummy with the fixed length. 
  With a every call_back you can then set the updated value to be send.

  2) Use notify to send data, but that will only send as many bytes that fit in the agreed MTU size between peripheral
  and central after connect. The default size is 23. Unfortunatly there is no library call to obtain
  the agreed MTU size in the standard ArduinoBLE, but it is in ArduinoBLE_P (see example13 and example14)

  3) First set a new value with the correct size on the characteristic. Then the peripheral uses a NOTIFY characteristic to 
  send SHORT message the central to indicate that a read characteristic is ready to be read.

  --------------------------------------------------------------------------------------------------------
  
  This example will demonstrate option 3.
  
  When this central requests a new message, it will be send in blocks of MAXBLOCKSIZE length.
  On each command (request or cancel) from the centralthe peripheral will send RECEIVED_OK.  After the request has
  been received the peripheral will then write the first block,send a notification that the block is ready to be read. 
  This central will then read the block and if the CRC is correct send a RECEIVED_OK and request the next block.
  Else it will request to resend the block.
 
  This will go on until the full message length (spread over multiple blocks). It is then displayed.
 
  Using this example12 you can achieve the following transfer speed:

  
  blocksize
  100 : it takes about 2900 mS to sent a data packet of 874 bytes.
  200 : it takes about 1450 mS to sent a data packet of 874 bytes.
  300 : it takes about 870 mS to sent a data packet of 874 bytes.(only ArduinoBLE_P !)
  400 : it takes about 848 mS to sent a data packet of 874 bytes.(only ArduinoBLE_P !)
  500 : it takes about 507 mS to sent a data packet of 874 bytes.(only ArduinoBLE_P !)

  Above 200 (actually 239) the packet does not fit in a single agreed MTU-size. AruindoBLE_P has been adjusted
  to take care of that !!
  We can't do more than 512 (maximum for ArduinoBLE)

*/

# include "defines.h"

///////////////////////////////////////////////////////
// Program defines
///////////////////////////////////////////////////////
#define MAGICNUM            0XCF      // should be byte 0 in new message
#define MAXBLOCKSIZE        500       // size of block buffer
#define RETRYCOUNT          5         // number of retry for better block handling

// Commands
#define NO_COMMAND          0         // No action Pending
#define REQ_NEW_MESSAGE     1         // central : new message req
#define CANCEL_MESSAGE      2         // Peripheral / Central : cancel and disgard message
#define NEW_BLOCK_AVAILABLE 3         // peripheral: to indicate new block is ready
#define RECEIVED_OK         4         // Peripheral / Central : acknowledge on good receive latest command
#define REQ_NEW_BLOCK       5         // Central : request new block  
#define RCD_CMPLT_MSG       6         // !!central : complete message received
// errors
#define TIMEOUT_BLOCK       7         // !!central : timeout, req to resend
#define INVALID_BLOCK       8         // central : invalid blocknumber
#define CRC_BLOCK           9         // central : CRC error, req to resend
#define REQ_INVALID        10         // Peripheral / Central : Did not request out of sequence
#define MAGIC_INVALID      11         // Central : Invalid first block
#define ERR_INTERNAL       12         // Central: internal error

///////////////////////////////////////////////////////
// Program variables
///////////////////////////////////////////////////////
uint8_t BlockCounter = 0;             // keep track of current block to send
int16_t TotalMessageLength = 0;       // keep track of total message bytes left to send
uint8_t *MessageBuf = NULL;           // holds the complete received message  
uint16_t MessageWriteOffset = 0;      // current write offset for message
uint8_t BlockBuf[MAXBLOCKSIZE];       // Data buffer from Bluetooth to user level
uint8_t PeripheralCmd = NO_COMMAND;   // latest command from peripheral
uint8_t PendingCommand = NO_COMMAND;  // latest command send to peripheral
uint8_t SetCommandNext = NO_COMMAND;  // Command PENDING to send in near future
char    input[10];                    // keyboard input buffer
int     inpcnt = 0;                   // keyboard input buffer length
uint8_t RetryErrorCnt = 0;            // keep track on repeated block request
bool    MessageComplete= false;       // if true complete message has been received. Ready to action / display

unsigned long st;
///////////////////////////////////////////////////////
// NO NEED FOR CHANGES BEYOND THIS POINT
///////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.print("\nExampe12 central RW_Notify. Compiled on: ");
  Serial.println(__TIME__);

  GetGoing();
}

void loop() {

  static uint8_t loopcnt = 0;
  
  // poll and check connect (in BLE_Comm)
  if (!CheckConnect()) {
    SERIAL_PORT.println(F("Disconnected\r"));
    SetCommandNext = NO_COMMAND;
    PeripheralCmd = NO_COMMAND;
    PendingCommand = NO_COMMAND;
    TotalMessageLength = 0;
     GetGoing();
  }

  // handle any keyboard input
  if (SERIAL_PORT.available()) {
    while (SERIAL_PORT.available()) handle_input(SERIAL_PORT.read());
  }

  // Handle (potential) input received from peripheral
  HandlePeripheralCmd();
  
  //any pending command to send
  if (SetCommandNext != NO_COMMAND){
    if (loopcnt++ > 1) {
      SendCommand(SetCommandNext);
      SetCommandNext = NO_COMMAND;
      loopcnt = 0;
    }
  }

  // indicates we have the complete message
  if (MessageComplete){
    DisplayMessage();
    TotalMessageLength = 0;
    MessageComplete = false;
  }
}

/**
 * start the BLE connection
 */
void GetGoing()
{
  // BLE init & wait for connection
  Start_BLE();                    // in BLE_Comm

  Serial.print(F("Using blocksize : "));
  Serial.println(MAXBLOCKSIZE);

  display_menu();
  
  Serial.println(F("Ready to go"));
}

/**
 * take action on command received from peripheral
 */
void HandlePeripheralCmd()
{
  // if No command received from peripheral
  if (PeripheralCmd == NO_COMMAND) return;

  switch (PeripheralCmd){
    
    case NEW_BLOCK_AVAILABLE:
#ifdef BLE_SHOW_DATA 
        SERIAL_PORT.println(F("New Data"));
#endif
        PeripheralCmd = NO_COMMAND;
        ReadDataBlock();            // will be acknowledged if block is good or bad).
      break;

    case RECEIVED_OK:
#ifdef BLE_SHOW_DATA 
      SERIAL_PORT.println(F("Got OK"));
#else
      // show still alive
      SERIAL_PORT.print(".");
#endif
      switch (PendingCommand) {     // this was previous command sent by us

        case CRC_BLOCK:
        case INVALID_BLOCK:
        case ERR_INTERNAL:
          RetryBlock();
          break;
          
        case CANCEL_MESSAGE:
          TotalMessageLength = 0;
          // fall through
        
        default:
          PeripheralCmd = NO_COMMAND;
          PendingCommand = NO_COMMAND;
      }
      break;

    case CANCEL_MESSAGE:
      SERIAL_PORT.println(F("Cancel Message request from peripheral"));
      SendCommand(RECEIVED_OK);
      PeripheralCmd = NO_COMMAND;
      PendingCommand = NO_COMMAND;
      SetCommandNext = NO_COMMAND;
      TotalMessageLength = 0;
      break;

    case REQ_INVALID:
      SERIAL_PORT.println(F("Invalid Request command from peripheral"));
      SendCommand(RECEIVED_OK);
      PendingCommand = NO_COMMAND;
      PeripheralCmd = NO_COMMAND;
      break;
  }
}

/** 
 *  Retry new block after error only for RETRYCOUNT
 */
void RetryBlock()
{
  if (RetryErrorCnt++ < RETRYCOUNT) 
    SendCommand(REQ_NEW_BLOCK);
  else {
    SERIAL_PORT.println(F("Retry count exceeded. Giving up"));
    SendCommand(CANCEL_MESSAGE);
  }
}

/**
 * send command on Write characteristic
 * Byte[0] = MAGICNUM
 * Byte[1] = command
 */
void SendCommand(uint8_t cmd){
  uint8_t s[2];
  s[0] = MAGICNUM;
  s[1] = cmd;
  TXData(s,2);

  // set pending command
  PendingCommand = cmd;
}

/**
 * Read block from peripheral
 * 
 * 
 * The total message can be split across multiple blocks.
 * The first byte in a the first package of a new messages will indicate the
 * total length of the message.
 */
void ReadDataBlock()
{
  uint16_t i = 0;                // offset to start copy data

  // read Block
  int ret = RXData(BlockBuf, MAXBLOCKSIZE);

  if (! ret) {
    SERIAL_PORT.println("Error: could not read Block\n");
    SendCommand(ERR_INTERNAL);
    return;    
  }

  // start of new message ?
  if (TotalMessageLength == 0) {
st = millis();
    // check for correct packet start
    if (BlockBuf[i++] != MAGICNUM)  {
      SERIAL_PORT.println("Invalid magic\n");
      SendCommand(MAGIC_INVALID);
      return;
    }
  
    // get total message size
    TotalMessageLength = BlockBuf[i++] << 8;
    TotalMessageLength = TotalMessageLength |BlockBuf[i++];

    // if previous message pending
    if (MessageBuf)  free(MessageBuf);

    // allocate space.
    MessageBuf = (uint8_t  *) malloc(TotalMessageLength);

    if (! MessageBuf) {
      SERIAL_PORT.println(F("Error: could not obtain memory\n"));
      SendCommand(ERR_INTERNAL);
      return;
    } 

    BlockCounter = 0;
    MessageWriteOffset = 0;
    
#ifdef BLE_SHOW_DATA 
    SERIAL_PORT.print(F("Total Data length in this message = "));
    SERIAL_PORT.println(TotalMessageLength);
#endif   
  }

  // for any block
  if ( BlockCounter++ != BlockBuf[i++]){
    SERIAL_PORT.println(F("Error: Invalid block\n"));
    SendCommand(INVALID_BLOCK);
    return;    
  }

  uint16_t ReceiveBufLen = BlockBuf[i++] << 8;
  ReceiveBufLen = ReceiveBufLen |BlockBuf[i++];

  // the total length is content length + CRC
  if (ret != ReceiveBufLen + 2 ){
    SERIAL_PORT.println(F("Error: Invalid length\n"));
    SendCommand(INVALID_BLOCK);
    return;
  }
  
#ifdef BLE_SHOW_DATA  
  SERIAL_PORT.print(F("Current Block len = "));
  SERIAL_PORT.println(ReceiveBufLen);
#endif

  // save current message location in case of CRC error
  uint16_t SaveMessageWriteOffset = MessageWriteOffset;
  
  // copy data of this packet in the receive buffer
  for (; i < ReceiveBufLen ; i++) {
    // prevent buffer overrun
    if (MessageWriteOffset < TotalMessageLength)
        MessageBuf[MessageWriteOffset++] = BlockBuf[i];
  }

  // get CRC of message
  uint16_t RcvCRC = BlockBuf[i++] << 8;
  RcvCRC |= BlockBuf[i++]; 

  if (RcvCRC != calc_crc(BlockBuf,i-2)) {
    SERIAL_PORT.println(F("CRC error"));
    SendCommand(CRC_BLOCK);
    
#if 0
    uint8_t k = 0;
    for(uint16_t j=0; j<i;j++){
      SERIAL_PORT.print(BlockBuf[j],HEX);
      SERIAL_PORT.print(" ");
      if(k++ > 80){
        SERIAL_PORT.println();
        k=0;
      }
    }
    SERIAL_PORT.print("\n");
#endif    
    // restore write offset
    MessageWriteOffset = SaveMessageWriteOffset;
    return;  
  }

  // we had a good block and communication
  SendCommand(RECEIVED_OK);
  RetryErrorCnt = 0;

#ifdef BLE_SHOW_DATA  
  SERIAL_PORT.print(F("MessageWriteOffset: "));
  SERIAL_PORT.print(MessageWriteOffset);  
  SERIAL_PORT.print(F(" TotalMessageLength: "));
  SERIAL_PORT.println(TotalMessageLength); 
#endif

  // did we receive the complete message.
  if (MessageWriteOffset >= TotalMessageLength) {
    MessageComplete = true;       // indicate message received
    Serial.printf("It took %d mS\n", millis() -st);
    SetCommandNext = RCD_CMPLT_MSG;
  }
  else {
    // first allow RECEIVED_OK to be send
    SetCommandNext = REQ_NEW_BLOCK;
  }
}

/**
 * Hand-off the notify received from the peripheral
 * 
 * Hand-off the data and let the "stack" clear with all the returns pending
 * to ArduinoBLE functions to prevent stack-overrun. 
 * The handling of the command triggered once returned in loop()
 */
void HandlePeripheralNotify(const uint8_t *buf, uint16_t len)
{
#ifdef BLE_SHOW_DATA  
  SERIAL_PORT.print(F("HandlePeripheralNotify: Magicnum 0x"));
  SERIAL_PORT.print(buf[0], HEX);
  SERIAL_PORT.print(F(" command 0x"));
  SERIAL_PORT.println(buf[1], HEX);
#endif
    
  // check for correct packet start
  if (buf[0] != MAGICNUM) {
    SendCommand(MAGIC_INVALID);
    return;
  }

  PeripheralCmd = buf[1];
}

///////////////////////////////////////////////////////////////////////////
// local routines
///////////////////////////////////////////////////////////////////////////
/**
 * handle keyboard input
 */
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
        SERIAL_PORT.print(F("Request message from peripheral\n"));
        TotalMessageLength = 0;
        RetryErrorCnt = 0;
        SendCommand(REQ_NEW_MESSAGE);
        break;
    
    case '2':
        SERIAL_PORT.print(F("Cancel message from peripheral\n"));
        SendCommand(CANCEL_MESSAGE);
        break;

    case '3':
        SERIAL_PORT.println(F("Disconnect request\r"));
        BLE.disconnect();
        WaitToReconnect();
        // reset keyboard buffer
        inpcnt = 0;
        return;
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

void display_menu()
{
  SERIAL_PORT.println(F("1. Request message from peripheral"));
  SERIAL_PORT.println(F("2. Cancel message from peripheral"));
  SERIAL_PORT.println(F("3. Disconnect from peripheral"));
}

/**
 * do something with the message
 */
void DisplayMessage() {
  int i,j;
  SERIAL_PORT.println("\nThe following complete message has been received:");

  for (i = 0, j = 0; i < TotalMessageLength ; i++, j++){
    if (j == 80) {
      j=0;
      SERIAL_PORT.println();
    }

    SERIAL_PORT.print((char) MessageBuf[i]);
  }
  
  SERIAL_PORT.println();
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
  while (! SERIAL_PORT.available()) CheckConnect();

  // clear buffer
  while (SERIAL_PORT.available()) {
    SERIAL_PORT.read();
    delay(100);
  }
}  

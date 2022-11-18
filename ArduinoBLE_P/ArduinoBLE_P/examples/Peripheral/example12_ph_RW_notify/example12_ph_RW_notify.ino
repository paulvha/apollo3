// CHANGE ArduinoBLE_P TO ArduinoBLE WHEN USING VERSION 2.X OF SPARKFUN LIBRARY

#include <ArduinoBLE_P.h>
 
/*
  This sketch demostrates the sending and receiving of data between 2 BLE devices
  It acts as the peripheral, thus it will advertise the service and characteristics

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
  the agreed MTU size.

  3) First set a new value with the correct size on the characteristic. Then the peripheral uses a NOTIFY characteristic to 
  send SHORT message the central to indicate that a read characteristic is ready to be read.

  --------------------------------------------------------------------------------------------------------
  
  This example will demonstrate option 3,

  When requested by a central it will send a large datapacket of TOTALMESSAGESIZE bytes on a characteristic
  that can hold MAXBLOCKSIZE bytes. 
  So we will create blocks of max. MAXBLOCKSIZE bytes that include checks to have a reliable exchange. 
  Once a block is ready to be read, a notification will be send to the central. The central will read and check for 
  correct block and provide feedback on success and request the next block. 
  If not correct it will the peripheral know the  error and can ask for resend. 
  This will go on until the complete TOTALMESSAGESIZE has been received by the central.
*/

///////////////////////////////////////////////////////
// Program defines
///////////////////////////////////////////////////////
#define TOTALMESSAGESIZE  874         // define the size of the complete message (example)
#define MAXBLOCKSIZE      100         // define the maximum block size
#define MAGICNUM          0XCF        // should be byte 0 in new message

// commands
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

// Uncomment to see progress / debug information
//#define BLE_SHOW_DATA 1

///////////////////////////////////////////////////////
// Program variables
///////////////////////////////////////////////////////
uint8_t BlockCounter = 0;             // keep track of current block to send
int16_t TotalMessageLength;           // keep track of total message bytes left to send
uint8_t BlockContent[MAXBLOCKSIZE];
uint8_t BlockContentLength;
uint8_t PendingCommand;               // expect RECEIVED_OK from central on this command
uint8_t CentralCmd;                   // commands/feedback received from central.

// Base to create sliding complete message
char Test[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqurstuvwxyz!@#$%^&*()_+";
uint8_t Test_offset=0;

char     input[10];                 // keyboard input buffer
int      inpcnt = 0;                // keyboard input buffer length

///////////////////////////////////////////////////////
// BLE defines
///////////////////////////////////////////////////////
const char BLE_PERIPHERAL_NAME[] = "R_W_Notify BLE";

#define SERVICE "9e400001-b5a3-f393-e0a9-e50e24dcca9e"

// create characteristic and allow remote device to read and write
#define CHARACTERISTIC_R_UUID "9e400002-b5a3-f393-e0a9-e50e24dcca9e" // receive feedback
#define CHARACTERISTIC_W_UUID "9e400003-b5a3-f393-e0a9-e50e24dcca9e" // Send data characteristic
#define CHARACTERISTIC_N_UUID "9e400004-b5a3-f393-e0a9-e50e24dcca9e" // Notify characteristic

///////////////////////////////////////////////////////
// BLE variables
///////////////////////////////////////////////////////

// create the service
BLEService Service(SERVICE);

// create Write Characteristics
BLECharacteristic W_Characteristic(CHARACTERISTIC_W_UUID, BLERead | BLEWrite, MAXBLOCKSIZE);

// create feedback Characteristic
BLECharacteristic R_Characteristic(CHARACTERISTIC_R_UUID, BLERead | BLEWrite, 5);

// create notify
BLECharacteristic N_Characteristic(CHARACTERISTIC_N_UUID, BLERead | BLENotify, 5);

///////////////////////////////////////////////////////
// NO NEED FOR CHANGES BEYOND THIS POINT
///////////////////////////////////////////////////////

void setup() {
   Serial.begin(115200);
   delay(1000);
   Serial.print("\nExampe12 Peripheral RW_notify. Compiled on: ");
   Serial.println(__TIME__);

  ///////////////////////////////////////////////////
  // BLE SETUP START
  ///////////////////////////////////////////////////
  // begin initialization
  if (!BLE.begin()) {
    Serial.println(F("starting BLE failed! freeze\r"));
    while (1);
  }

  // set the local name peripheral advertises
  BLE.setLocalName(BLE_PERIPHERAL_NAME);

  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(Service);

  // add the characteristics to the service
  Service.addCharacteristic(R_Characteristic); // characteristic that provides feedback from central
  Service.addCharacteristic(W_Characteristic); // characteristic to send data
  Service.addCharacteristic(N_Characteristic); // characteristic to notify central with commands

  // add the service
  BLE.addService(Service);

  // set connection handlers
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // start advertising
  BLE.advertise();

  // called when read is performed by remote
  W_Characteristic.setEventHandler(BLERead, (BLECharacteristicEventHandler) ReadCallBack);

  // called when something written by remote (and thus we read)
  R_Characteristic.setEventHandler(BLEWritten, (BLECharacteristicEventHandler) WriteCallBack);

  ///////////////////////////////////////////////////
  // BLE SETUP END
  ///////////////////////////////////////////////////
  
  Serial.println(F("Peripheral ready to go"));
}

void loop() {
  static uint8_t LoopCnt =0;
  
  // wait for connection
  while (! BLE.connected()){
    BLE.poll();
    delay(1000);     // don't hammer
  }
  
  // handle any keyboard input
  if (Serial.available()) {
    while (Serial.available()) handle_input(Serial.read());
  }

  if (CentralCmd != NO_COMMAND) {

    // wait a couple of BLE.poll() before acting
    if (LoopCnt++ > 3) {
      HandleCentralReq();
      LoopCnt = 0;
    }
  }
  delay(50);        // needed for BLE and don't hammer   
  BLE.poll();
}

/**
 * called when reading is being done by remote. 
 */
void ReadCallBack(BLEDevice *central, BLECharacteristic c) {
#ifdef BLE_SHOW_DATA 
    Serial.println(F("Read call_Back. Central is reading"));
#endif
    // no action only info
}

/**
 * called when a command has been written by the central. so we read
 * 
 * format 
 * Byte[0] = MAGICNUM
 * Byte[1] = command / feedback
 */
void WriteCallBack(BLEDevice *central, BLECharacteristic c) {
#ifdef BLE_SHOW_DATA   
  Serial.println(F("Central has written something."));
#endif

  int lenc = c.valueLength();
  uint8_t *valuec=(uint8_t*) c.value();

  // check for correct packet start
  if (valuec[0] != MAGICNUM) {
    Serial.print(F("Invalid magic number : 0x"));
    Serial.println(valuec[0],HEX);
    return;
  }
  
#ifdef BLE_SHOW_DATA 
  Serial.print("Received command (HEX) 0x");
  Serial.println(valuec[1]);
#endif

  // for now we only use ONE command
  CentralCmd = valuec[1];

  switch (CentralCmd){
    case RECEIVED_OK:
#ifdef BLE_SHOW_DATA 
      Serial.println("RECEIVED_OK");
#else
      Serial.print(".");      // show action
#endif
      HandlePendingCmd(); 
      break;
           
    case TIMEOUT_BLOCK:       // these are commands/errors from Central
    case CRC_BLOCK:
    case CANCEL_MESSAGE:   
    case INVALID_BLOCK:
    case REQ_NEW_MESSAGE:
    case MAGIC_INVALID:
    case ERR_INTERNAL:
    case REQ_NEW_BLOCK:    
      SendCommand(RECEIVED_OK);
      break;

    default:  // unknown
      SendCommand(REQ_INVALID);
      break;      
  }
}

/**
 * act on received command/errors from central
 */
void HandleCentralReq()
{  
  if (CentralCmd == REQ_NEW_BLOCK) {

    uint8_t ret = CreateDataToSend();
    // if complete message has been send
    if (ret == 0) {
      Serial.print(F("Sent Block "));
      Serial.println(BlockCounter);
    }
    else if (ret == 1) {
    
#ifdef BLE_SHOW_DATA 
      Serial.println(F("Complete message had been sent before"));
      Serial.println(F("No action now."));
#endif
      SendCommand(REQ_INVALID);
    }
    else if (ret == 2) {
      Serial.print(F("\nComplete message has now been sent in "));
      Serial.print(BlockCounter);
      Serial.println(F(" blocks."));
    }
  }
  
 else if (CentralCmd == CANCEL_MESSAGE){
    Serial.println(F("\nCancel current message."));
    TotalMessageLength = -1;
  }
 
  else if (CentralCmd == REQ_NEW_MESSAGE){
    Serial.println(F("Request new message."));
    TotalMessageLength = 0;
    
    // if complete message has not been send
    if (CreateDataToSend() != 1) {
    
#ifdef BLE_SHOW_DATA 
      Serial.println(F("New message block has been sent"));
#endif
    }
  }

  CentralCmd = NO_COMMAND;
}
/*
 * send a complete message of X bytes broken down in block of max 100 size 
 * 
 * Message format
 * 
 * block 0
 * byte 0  = MagicNum
 * Byte 1  = MSB total message length (uint16_t)
 * Byte 2  = LSB total message length
 * Byte 3 =  block number (starting 0) (uint8_t)
 * byte 4 =  block data length  (excluding 2 bytes CRC) (uint8_t)
 * byte 5 - N  data (N is MAX  MAXBLOCKSIZE -3)
 * byte N +1 = MSB CRC on ALL previous bytes in block (uint16_t)
 * byte N +2 = LSB CRC
 * 
 * 
 * blocks following
 * Byte 1  = current block number X (uint8_t)
 * Byte 2  = block data length  (excluding 2 bytes CRC) (uint8_t)
 * byte 5 - N  data (N is MAX  MAXBLOCKSIZE -3)
 * byte N +1 = MSB CRC on ALL previous bytes in block (uint16_t)
 * byte N +2 = LSB CRC
 * 
 * Protocol :
 * 
 * peripheral                   central
 * =============================================
 * Wait for request
 *                            send request for message
 * Send Received_OK             
 *                            Wait for notification
 * prepare block
 * add CRC
 * write block
 * write notification
 *                            receive block
 *                            check block Number and CRC
 *                            if not correct, request resend block#
 * resend block
 *                            else extract & store relevant data
 *                            write peripheral all Received_OK 
 *                            request next block
 * Send Received_OK             
 *                            Wait for notification
 * prepare block
 * add CRC
 * write block
 * write notification
 * 
 * return 
 *  0 ; message created and send
 *  1 ; complete message had been send before
 *  2 ; last block in message has been send
 * 
 */

uint8_t CreateDataToSend(){
  int i = 0 ,j;

  // complete message had been send already
  if (TotalMessageLength < 0)  return(1);
  
  // if start of new message
  if (TotalMessageLength == 0) {

    BlockContent[i++] = MAGICNUM;                       // set magicnum

    TotalMessageLength = TOTALMESSAGESIZE;

    BlockContent[i++] = TotalMessageLength >> 8 & 0xff;  // set MSB

    BlockContent[i++] = TotalMessageLength & 0xff;       // set LSB

    BlockCounter = 0;

    Test_offset=0;            // offset in source
  }

  // add content for any block 
  BlockContent[i++] = BlockCounter++;
  
  // remember position to add block datalength
  uint8_t LengthData = i++;   
 
  // add to content up to CRC (hence -2) OR Total message has been added
  for (; i < MAXBLOCKSIZE - 2 && TotalMessageLength-- > 0 ;i++) {

   // copy content to block
   BlockContent[i] = Test[Test_offset++];

   // loop around test source content
   if (Test_offset == sizeof(Test)) Test_offset = 0;
  }

  // add block length (excluding 2 bytes CRC)
  BlockContent[LengthData] = i;

  // calculate CRC over the curent block
  uint16_t BlockCrc = calc_crc(BlockContent,i);
 
   // add CRC
  BlockContent[i++] = BlockCrc >> 8 & 0xff;  // CRC MSB
  BlockContent[i++] = BlockCrc & 0xff;       // CRC LSB

  BlockContentLength = i;
  
#ifdef BLE_SHOW_DATA 
  Serial.println("Sending content: ");
  // display Hex
  for (int k = 0; k < i;k++){
    Serial.print(BlockContent[k],HEX);
    Serial.print(" ");
  }
  Serial.print("\nlength: ");
  Serial.println(i);
#endif
  SendBlock();

  // complete message has been send now
  if (TotalMessageLength < 0)  return(2);
  else return(0);
}

/**
 * Write a block to the characteristic and let the central
 * know it is available on Notify
 */
void SendBlock()
{
  
  if (! BLE.connected()){
    Serial.println(F("ERROR : NOT CONNECTED"));
    return;
  }

  // if valid Block
  if (BlockContentLength) {
    W_Characteristic.writeValue(BlockContent,BlockContentLength);

    // send notification new data is available
    SendCommand(NEW_BLOCK_AVAILABLE);
  }
}

/**
 * send command on notify
 * Byte[0] = MAGICNUM
 * Byte[1] = command
 */
void SendCommand(uint8_t cmd){
  uint8_t s[2];
  s[0] = MAGICNUM;
  s[1] = cmd;
  N_Characteristic.writeValue(s,2);

  // set pending command
  PendingCommand = cmd;
}

/**
 * we got a receive OK on previous command send by us
 */
void HandlePendingCmd() {

  switch (PendingCommand){
    case NEW_BLOCK_AVAILABLE:
    case CANCEL_MESSAGE:
    case REQ_INVALID:
#ifdef BLE_SHOW_DATA 
      Serial.println("Previous command acknowledged");
#endif
      break;

    default:
      Serial.println("Unexpected Receive_OK. Ignored");
  }

  PendingCommand = NO_COMMAND;
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

  // only the first character is used here
  switch (input[0]) {
    
    case '1' :    // start sending new message
      Serial.println(F("Request new message"));

      if (HandlePendingCmd == NO_COMMAND) {
        if (TotalMessageLength < 0) {
         TotalMessageLength = 0;
         CreateDataToSend();       
        }
        else {
          Serial.println(F("First send cancel current message"));
        }
      }
      else
        Serial.println(F("Waiting on Received_Ok from central"));
      break;
  
    case '2' :    // send command cancel
      Serial.println(F("Cancel current message\n"));
      SendCommand(CANCEL_MESSAGE);
      break;

    default :
      Serial.print("Unknown command: ");
      Serial.println(input[0]);
      break;
      
  }
  
  display_menu();

  // reset keyboard buffer
  inpcnt = 0;
}

void display_menu()
{
  Serial.println(F("1.  Start sending new message\r"));
  Serial.println(F("2.  Cancel current message\r"));
  Serial.println(F("3.  Disconnect\r"));
}

/**
 * called when central connects
 */
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  TotalMessageLength = 0;
  PendingCommand = NO_COMMAND;
}

/**
 * called when central disconnects
 */
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}

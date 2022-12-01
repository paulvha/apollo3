/*
 *
 *  GattLib - GATT Library
 *
 *  Copyright (C) 2016-2017  Olivier Martin <olivier@labapart.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 ****************************************************************************
 *
 * This example14_RW_N_mtu  started based on read_write.c example, but that ended
 * up to be only 5% of the RW_N_mtu_example.
 *
 * It has a number of command line options to demonstrate different approaches.
 *
 * This example is menu driven. It can scan based on provided BLE address or
 * BLE name. If not provided it will use a default name, which is the same
 * as the example14_ph_RW_Notify_MTU.ino.
 *
 * Once connected you can provide interactive request to:
 *  1)receive a message,
 *  2)cancel the message
 *  q)disconnect.
 *
 * The peripheral has also the same menu options.
 *
 * This example is much the same RW_N_example, but instead of a fixed blocksize
 * the blocksize is depending on the agreed MTU size.
 *
 * When you request a new message, it will be send in blocks of MTU-size.
 * Once requested a new message, the peripheral will send RECEIVED_OK.
 * The peripheral will then write the first block, send a notification that a block is ready to be read.
 * This central will then read the block and if the CRC is correct send a RECEIVED_OK and request the next block.
 * Else it will request to resend the block.
 *
 * This will go on until the full message length (spread over multiple blocks). It is then displayed.
 * If filename was provided on command line, it will be saved in the file as well.
 *
 * The challenge is that we do NOT know the agreed MTU size. There is no dbus-call available to obtain that locally.
 * So we send a REQ_BLOCKSIZE to the peripheral, which responds with a RSP_BLOCKSIZE. Then we know the mtuSIZE and
 * can allocate the right memory for that.
 *
 * ===================================================================================
 *
 * Message format
 *
 * Block 0
 * Byte 0  = MagicNum
 * Byte 1  = MSB total message length (uint16_t)
 * Byte 2  = LSB total message length
 * Byte 3 =  block number (starting 0) (uint8_t)
 * Byte 4 =  MSB block data length  (excluding 2 bytes CRC) (uint16_t)
 * Byte 5 =  LSB block data length
 * byte 5 - N  data (N is MAX  MAXBLOCKSIZE -3)
 * Byte N +1 = MSB CRC on ALL previous bytes in block (uint16_t)
 * Byte N +2 = LSB CRC
 *
 *
 * blocks following
 * Byte 1  = current block number X (uint8_t)
 * Byte 2  = block data length  (excluding 2 bytes CRC) (uint8_t)
 * byte 3 - N  data (N is MAX  MTU size -2)
 * byte N +1 = MSB CRC on ALL previous bytes in block (uint16_t)
 * byte N +2 = LSB CRC
 *
 * protocol sequence example
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
 *
 *.....................................
 *
 * See usage() for different options
 *
 * paulvha/ November 2022 / version 1.0
 *
 */

# include <assert.h>
# include <glib.h>
# include <stdio.h>
# include <stdlib.h>
# include <signal.h>
# include <getopt.h>
# include "gattlib.h"
# include <stdbool.h>
# include <time.h>
# include <ctype.h>


//////////////////////
// BLE defines
//////////////////////
#define SERVICE "9e400001-b5a3-f393-e0a9-e14e24dcca9e"

//  characteristic read, notify and write
#define CHARACTERISTIC_R_UUID "9e400002-b5a3-f393-e0a9-e14e24dcca9e" // receive feedback
#define CHARACTERISTIC_W_UUID "9e400003-b5a3-f393-e0a9-e14e24dcca9e" // Send data characteristic
#define CHARACTERISTIC_N_UUID "9e400004-b5a3-f393-e0a9-e14e24dcca9e" // Notify characteristic

// default devicename if NO BLE address or devicename provided on command line
char *default_dev_name = "Example14 BLE";

//////////////////////
// program defines
/////////////////////
#define MAGICNUM 	         0xcf 	// byte 0 /start of each message
#define RETRYCOUNT          5   	// number of retry for better block handling

// Commands
#define NO_COMMAND          0   	// No action Pending
#define REQ_NEW_MESSAGE     1   	// central : new message req
#define CANCEL_MESSAGE      2   	// Peripheral / Central : cancel and disgard message
#define NEW_BLOCK_AVAILABLE 3   	// peripheral: to indicate new block is ready
#define RECEIVED_OK         4   	// Peripheral / Central : acknowledge on good receive latest command
#define REQ_NEW_BLOCK       5   	// Central : request new block
#define RCD_CMPLT_MSG       6   	// central : complete message received
#define REQ_BLOCKSIZE      13     // central: get block size to use (= MTU)
#define RSP_BLOCKSIZE      14     // peripheral : send block size to use (= MTU)

// errors
#define TIMEOUT_BLOCK       7   	// !!central : timeout, req to resend
#define INVALID_BLOCK       8  	 	// central : invalid blocknumber
#define CRC_BLOCK           9   	// central : CRC error, req to resend
#define REQ_INVALID        10   	// Peripheral / Central : Did not request out of sequence
#define MAGIC_INVALID      11   	// Central : Invalid first block
#define ERR_INTERNAL       12   	// Central: internal error

// keyboard operation
typedef enum {READMESSAGE, CANCELMESSAGE, NONE} operation_t;

///////////////////////
// program variables
///////////////////////
operation_t g_operation = NONE;
static uuid_t w_uuid, r_uuid,n_uuid;	// holds UUID's
gatt_connection_t* connection = NULL;
GMainLoop* loop = NULL;
void* adapter = NULL;
char* dev_addr = NULL;         		// holds the peripheral address
char* dev_name = NULL;         		// holds the peripheral name (if known)
char* StrFileName = NULL;      		// filename to store received Ascii string data
bool dev_discovered = false;   		// true = peripheral device is detected
bool TimeStamp = true;         		// true = add timestamp to output
bool DisplayCommaSep = false;  		// true = display comma separated
bool SHOW_scan = true;         		// true = show all scan discovered devices
int timeout = 5;              		// default scantime in seconds
uint8_t BlockCounter = 0;               // keep track of current block to send
int16_t TotalMessageLength = 0;       	// keep track of total message bytes left to send
uint8_t *MessageBuf = NULL;           	// holds the complete received message
uint16_t MessageWriteOffset = 0;      	// current write offset for message
uint8_t *BlockBuf = NULL;		            // holds the block received
uint8_t PendingCommand = NO_COMMAND;  	// latest command send to peripheral
uint8_t RetryErrorCnt = 0;            	// keep track on repeated block request
uint8_t mtuSize = 0;

/////////////////////////
// forward declarations
/////////////////////////
void SendCommand(uint8_t cmd);
void SaveToFile( char *fileName, uint8_t *buffer, uint16_t lenBuf);
void notification_handler(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data);
void RetryBlock();
void ReadDataBlock();
void HandleReceivedData();
uint16_t calc_crc(uint8_t *p, uint16_t lenb);

// for extra debug messages remove comments //
//#define BLE_DEBUG 1

/**
 * usage
 */
static void usage(char *argv[]) {
  fprintf(stderr, "%s  [-a adapter] [-b address] [-d devicename] [-t scan timeout]\n", argv[0]);
  fprintf(stderr, "    [-f filename] [-s]\n\n");

  fprintf(stderr, "-a  option to provide local BLE adapter name.\n");
  fprintf(stderr, "-b  provide the remote BLE address instead of name.\n");
  fprintf(stderr, "-d  provide the remote BLE devicename instead of address.\n");
  fprintf(stderr, "-f  provide the filename to store received data.\n");
  fprintf(stderr, "-s  will only display scan result for device found.\n");
  fprintf(stderr, "-t  set scan timeout (default %d seconds)\n", timeout);
  fprintf(stderr, "\nif NO device name and no address the default devicename is '%s'\n", default_dev_name);

  exit(-1);
}

/**
 * close out correctly
 * @param ret : the exit value
 */
void cleanup(int ret) {

  // if called when loop is not active
  if (! loop) {

    // check that a connection needs to be cancelled
    if (connection) {

      // this can cause a warning that can be ignored
      gattlib_disconnect(connection,true);
    }

    if (MessageBuf) free(MessageBuf);
    if (BlockBuf) free (BlockBuf);

    fprintf(stderr,"Bye, exit code: %d\n", ret);
    exit(ret);
  }
  else // stop loop first
    g_main_loop_quit(loop);
}

/**
 * call back if device is discovered
 *
 * if dev_name provided, try to retrieve the BLE address
 * if dev_addr provided, check device can be found
 */
static void ble_discovered_device(const char* addr, const char* name) {

  // If device is not discovered, but we have a dev_addr
  // this address was provided on the command line.
  // We check for match to make sure the peripheral is in reach.
  if (! dev_discovered && dev_addr != NULL) {

    if (strcmp(addr,dev_addr) == 0) {
      if (name) fprintf(stderr, "FOUND '%s': '%s'.\n", addr, name);
      else fprintf(stderr, "FOUND device '%s'.\n", addr);
      dev_discovered = true;
      return;
    }
  }

  // if scanned device has a name
  if (name) {

    // if devicename was provided and not found yet
    if (dev_name && ! dev_discovered) {
      // try to get BLE address of the devicename we are looking for
      if (strcmp(name,dev_name) == 0) {
        fprintf(stderr, "FOUND '%s' on '%s'.\n",name, addr);
        dev_addr = strdup(addr);
        dev_discovered = true;
      }
    }
    else if (SHOW_scan)
      fprintf(stderr,"Discovered '%s' - '%s'.\n",name, addr);

  } else if (SHOW_scan)
      fprintf(stderr,"Discovered '%s'.\n", addr);
}

/**
 * Start scanning for the device either based on provided BLE address
 * or else look for dev_name and store the BLE the address
 */
int Find_device(){
  int ret;

  ret = gattlib_adapter_open(NULL, &adapter);
  if (ret) {
    fprintf(stderr, "ERROR: Failed to open adapter.\n");
    return -1;
  }

  if (dev_name) fprintf(stderr,"Start scanning for '%s' (%d seconds)\n\n", dev_name, timeout);
  else fprintf(stderr,"Start scanning for '%s' (%d seconds)\n\n", dev_addr, timeout);

  ret = gattlib_adapter_scan_enable(adapter, ble_discovered_device, timeout);

  if (ret) {
    fprintf(stderr, "ERROR: Failed to scan.\n");
    return -1;
  }

  gattlib_adapter_scan_disable(adapter);
  fprintf(stderr,"Scan completed.\n");
  return 0;
}

/**
 * This will be called when peripheral disconnected
 */
void disconnect_handler(gatt_connection_t* connection)
{
  fprintf(stderr,"Peripheral disconnected.\n");
  cleanup(6);
}

/**
 * This will be called when a notification is arriving
 */
void notification_handler(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data)
{
#if (0)
    /***** below left in for debug ****/
    printf("Notification Handler: ");

    // add the UUID
    char uuid_str[MAX_LEN_UUID_STR + 1];
    gattlib_uuid_to_string(uuid, uuid_str, sizeof(uuid_str));
    printf("UUID %s: ", uuid_str);

    // DEBUG : display data raw
    for (int j = 0; j < data_length; j++) {
	printf("%02x ", data[j]);
    }
    printf("\n");

    //gattlib_rssi(connection);
    /********* end of debug ***********/
#endif

    // check for correct packet start to improve synchronisation
    if (data[0] != MAGICNUM ){
      fprintf(stderr,"Invalid MAGICNUM.\n");
      SendCommand(MAGIC_INVALID);
      return;
    }

    // check on reveid command
    switch (data[1]){

      case NEW_BLOCK_AVAILABLE:
#ifdef BLE_DEBUG
	fprintf(stderr,"New Data block available\n");
#endif
        ReadDataBlock();
        break;

      case RSP_BLOCKSIZE:
        mtuSize = data[2];
        SendCommand(RECEIVED_OK);
        break;

      case RECEIVED_OK:
#ifdef BLE_DEBUG
        fprintf(stderr,"Got OK\n");
#else
        // show still alive
      fprintf(stderr,".");
#endif
      switch (PendingCommand) {

        case CRC_BLOCK:
        case MAGIC_INVALID :
        case INVALID_BLOCK:
        case ERR_INTERNAL:
        RetryBlock();
        break;

        case CANCEL_MESSAGE:
          TotalMessageLength = 0;
          // fall through
        case REQ_BLOCKSIZE:
          // fall through
        default:
          RetryErrorCnt = 0;		// clear any retry
          PendingCommand = NO_COMMAND;
      }
      break;

    case CANCEL_MESSAGE:
      fprintf(stderr,"Cancel Message request from peripheral\n");
      SendCommand(RECEIVED_OK);
      TotalMessageLength = 0;
      break;

   case REQ_INVALID:
      fprintf(stderr,"Invalid Request command from peripheral\n");
      SendCommand(RECEIVED_OK);
      break;
  }
}

/**
 * send command on Write characteristic to peripheral
 * Byte[0] = MAGICNUM
 * Byte[1] = command
 * Byte[2] = optional parameter
 */
void SendCommand(uint8_t cmd)
{
  uint8_t s[3];
  s[0] = MAGICNUM;
  s[1] = cmd;
  s[2] = 0x0;  // optional parameter to exchange (future)

  if (gattlib_write_char_by_uuid(connection, &w_uuid, s, 3) != 0) {
    fprintf(stderr, "Failed to send command.\n");
    return;
  }

  // set pending command
  PendingCommand = cmd;
}

/**
 *  Retry new block after error only for RETRYCOUNT times
 */
void RetryBlock()
{
  if (RetryErrorCnt++ < RETRYCOUNT)
    SendCommand(REQ_NEW_BLOCK);
  else {
    fprintf(stderr,"Retry count exceeded. Given up\n");
    SendCommand(CANCEL_MESSAGE);
  }
}

/**
 * Read blocks from peripheral
 *
 * The total message can be split across multiple blocks.
 *
 * See top of file for format and protocol
 *
 */
void ReadDataBlock()
{
  uint16_t i = 0;                // offset to read content

  size_t len = (size_t) mtuSize;

  // if no memory allocated for block
  if (!BlockBuf) {

    if (! len) {
      fprintf(stderr,"BlockSize not known\n");
      SendCommand(ERR_INTERNAL);
      return;
    }

    // allocate memory for block
    BlockBuf = malloc (mtuSize);

    // if not succesfull
    if (!BlockBuf){
      fprintf(stderr,"Could not allocate %d size memory\n",mtuSize);
      SendCommand(ERR_INTERNAL);
      return;
    }
  }

  // read Block
  if (gattlib_read_char_by_uuid(connection, &r_uuid, BlockBuf, &len) != 0){
    fprintf(stderr,"Error during trying to reading data\n");
    SendCommand(ERR_INTERNAL);
    return;
  }

  // start of new message ?
  if (TotalMessageLength == 0) {

    // check for correct packet start
    if (BlockBuf[i++] != MAGICNUM)  {
      fprintf(stderr,"Invalid magic\n");
      SendCommand(MAGIC_INVALID);
      return;
    }

    // get total message size
    TotalMessageLength = BlockBuf[i++] << 8;
    TotalMessageLength = TotalMessageLength |BlockBuf[i++];

    // if previous message buffer pending (new message might have different length)
    if (MessageBuf) free(MessageBuf);

    // allocate space.
    MessageBuf = (uint8_t *) malloc(TotalMessageLength);

    if (! MessageBuf) {
      fprintf(stderr,"Error: could not obtain memory\n");
      SendCommand(ERR_INTERNAL);
      return;
    }

    if (BlockBuf[i++] != mtuSize) {
      fprintf(stderr,"Error: MTU sizes do not match\n");
      SendCommand(ERR_INTERNAL);
      return;
    }

    BlockCounter = 0;
    MessageWriteOffset = 0;

#ifdef BLE_DEBUG
    fprintf(stderr,"Total Data length in this message = %d\n",TotalMessageLength);
#endif
  }

  // for any block
  if ( BlockCounter++ != BlockBuf[i++]){
    fprintf(stderr,"Error: Invalid block\n");
    SendCommand(INVALID_BLOCK);
    return;
  }

  uint16_t ReceiveBufLen = BlockBuf[i++] << 8;     // block data length (excluding CRC)
  ReceiveBufLen = ReceiveBufLen |BlockBuf[i++];

  // the total length is content length + 2 bytes CRC
  if (len != ReceiveBufLen + 2 ){
    fprintf(stderr,"Error: Invalid length\n");
    SendCommand(INVALID_BLOCK);
    return;
  }

#ifdef BLE_DEBUG
  fprintf(stderr,"Current Block len = %d\n", ReceiveBufLen );
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
    fprintf(stderr,"CRC error\n");
    SendCommand(CRC_BLOCK);
#if 0
    for(uint8_t j=0; j<i ; j++){
      fprintf(stderr,"%x ",BlockBuf[j]);
    }
    fprintf(stderr,"\n");
    while(1);
#endif
    // restore write offset
    MessageWriteOffset = SaveMessageWriteOffset;
    return;
  }

  // we had a good block and communication
  SendCommand(RECEIVED_OK);

#ifdef BLE_DEBUG
  fprintf(stderr,"MessageWriteOffset: %d, TotalMessageLength %d\n",MessageWriteOffset, TotalMessageLength);
#endif

  // we did not receive the complete message.
  if (MessageWriteOffset < TotalMessageLength) {
    RetryErrorCnt = 0;		// clear any retry
    SendCommand(REQ_NEW_BLOCK);
  }
  else {
    SendCommand(RCD_CMPLT_MSG);
    HandleReceivedData();
  }
}

/**
 * Handle any received data
 */
void HandleReceivedData()
{
  int i,j;
  fprintf(stderr,"\nThe following complete message has been received\n");

  for (i = 0, j = 0; i < TotalMessageLength ; i++, j++){
    if (j == 80) {
      j=0;
      printf("\n");
    }
    printf("%c", MessageBuf[i]);
  }
  printf("\n");

  // Do we need to save to file ?
  if (StrFileName) SaveToFile(StrFileName, MessageBuf, TotalMessageLength);

  // reset
  TotalMessageLength = 0;
}

/**
 * this is called when contrl-C is pressed on the keyboard
 */
void my_signal_handler(int s){
   //fprintf(stderr,"Caught signal %d\n", s);
   cleanup(0);
}

/**
 * display keyboard / interactive commands possible
 */
static void usage_keyb() {

  fprintf(stderr, "1.  Request message\n");
  fprintf(stderr, "2.  Cancel message\n");
  fprintf(stderr, "q.  Quit program\n");
}

/**
 * save received data to a file
 *
 * @param fileName : the filename to use
 * @param buffer : content to write
 * @param lenBuf : length of buffer
 */
void SaveToFile( char *fileName, uint8_t *buffer, uint16_t lenBuf){
  FILE * pFile;

  // open file append
  pFile=fopen(fileName, "a");

  if (! pFile ) {
    fprintf(stderr, "Error opening file: %s.", fileName);
    return;
  }

  // save content to file
  for (uint16_t i = 0 ; i < lenBuf; i++)
    fprintf(pFile,"%c",buffer[i]);

  // add EOL
  fprintf(pFile,"\n");

  // close file
  fclose(pFile);
}

/**
 * Process keyboard input
 */
static gboolean
handle_keyboard (GIOChannel * source, GIOCondition cond)
{
  gchar *str = NULL;

  if (g_io_channel_read_line (source, &str, NULL, NULL,
    NULL) != G_IO_STATUS_NORMAL) {
    return TRUE;
  }

  // requesting to stop
  if (str[0] == 'q' || str[0] == 'Q') {
    g_free(str);
    cleanup(0);
    return TRUE;
  }
  // if only <enter>
  else if (str[0] == 0x0a) {
    usage_keyb();
    g_free(str);
    return TRUE;
  }

  if (str[0] == '1')      g_operation = READMESSAGE;

  else if (str[0] == '2') g_operation = CANCELMESSAGE;

  else {
    fprintf(stderr, "Invalid request %d\n", str[0]);
    g_free(str);
    return TRUE;
  }

  // if start reading new message
  if (g_operation == READMESSAGE){
    if (TotalMessageLength != 0) {
      fprintf(stderr, "Can not start reading message. Cancel first\n");
    }
    else {
      fprintf(stderr, "Request sending new message\n");
      SendCommand(REQ_NEW_MESSAGE);
    }
  }

  // cancel current message
  else if (g_operation == CANCELMESSAGE) {
    fprintf(stderr, "Cancel message\n");
    SendCommand(CANCEL_MESSAGE);
  }

  g_operation = NONE;
  g_free (str);
  return TRUE;
}

int main(int argc, char *argv[]) {

  int ret, opt;
  GIOChannel *io_stdin;

  // capture cntrl-c
  signal (SIGINT,my_signal_handler);

  /* parse commandline */
    while ((opt = getopt(argc, argv, "a:b:d:t:shf:")) != -1)
    {
    switch (opt) {
      case 'a':   // adapter
        adapter = strdup(optarg);
        break;

      case 'b':   // provided BLE address
        if (dev_name) {
          fprintf(stderr, "Provide EITHER peripheral devicename or BLE address. NOT both.\n");
          exit(-1);
        }
        dev_addr = strdup(optarg);
        break;

      case 'd':  // devicename to look for
        if (dev_addr) {
          fprintf(stderr, "Provide EITHER peripheral devicename or BLE address. Not both.\n");
          exit(-1);
        }
        dev_name = strdup(optarg);
        break;

      case 'f':   // provided filename to store data received
        StrFileName = strdup(optarg);
        break;

      case 't':  // set scan timeout
        timeout = (int) strtod(optarg, NULL);
        if (timeout < 2) {
          fprintf(stderr, "%d invalid timeout.\n", timeout);
          exit(-1);
        }
        break;

      case 's':  // set to reduce scan results display
        SHOW_scan = 0;
        break;

      case 'h':  // help
        usage(argv);
        break;

      default:
        fprintf(stderr, "Unknown option %d.\n", opt);
        usage(argv);
        break;
    }
  }

  // if no BLE address
  if (! dev_addr) {
    // AND no devicename : use default devicename
    if (! dev_name) dev_name = strdup(default_dev_name);
  }

  // set keyboard handling
  io_stdin = g_io_channel_unix_new (fileno (stdin));
  g_io_add_watch (io_stdin, G_IO_IN, (GIOFunc) handle_keyboard, NULL);

  // translate characteristic for us to write.
  if (gattlib_string_to_uuid(CHARACTERISTIC_R_UUID , strlen(CHARACTERISTIC_R_UUID) + 1, &w_uuid) < 0) {
    usage(argv);
    return 1;
  }

  // translate characteristic for us to read.
  if (gattlib_string_to_uuid(CHARACTERISTIC_W_UUID , strlen(CHARACTERISTIC_W_UUID) + 1, &r_uuid) < 0) {
    usage(argv);
    return 1;
  }

  // translate characteristic for us to receive notifications.
  if (gattlib_string_to_uuid(CHARACTERISTIC_N_UUID , strlen(CHARACTERISTIC_N_UUID) + 1, &n_uuid) < 0) {
    usage(argv);
    return 1;
  }

  // look for device
  ret = Find_device();
  if (ret) exit(1);

  if (! dev_discovered){
    if (dev_addr) fprintf(stderr,"Device with address '%s' was NOT discovered.\n", dev_addr);
    else fprintf(stderr,"Device with name '%s' was NOT discovered.\n", dev_name);
    exit(2);
  }

  fprintf(stderr,"Trying to connect.\n");

  connection = gattlib_connect(NULL, dev_addr, BDADDR_LE_RANDOM, BT_SEC_LOW, 0, 0);
  if (! connection) {
    if (dev_addr) fprintf(stderr,"Fail to connect to the bluetooth device %s.\n", dev_addr);
    else fprintf(stderr,"Fail to connect to the bluetooth device %s.\n",dev_name);
    exit(3);
  }

  fprintf(stderr,"Connected.\n");

  // register disconnect call back
  ret = gattlib_register_disconnect(connection, disconnect_handler);
  if (ret) {
    fprintf(stderr, "Fail to set disconnect callback.\n");
    cleanup(4);
  }

  // set notification call back
  gattlib_register_notification(connection, notification_handler, NULL);

  // enable notification
  ret = gattlib_notification_start(connection, &n_uuid);
  if (ret) {
    fprintf(stderr, "Fail to start notification.\n");
    cleanup(5);
  }

  // request agreed MTU-size to get the blocksize to use
  SendCommand(REQ_BLOCKSIZE);

  fprintf(stderr, "Ready to go.\n");

  usage_keyb();

  loop = g_main_loop_new(NULL, 0);
  g_main_loop_run(loop);

  // we are done
  fprintf(stderr, "Ending program\n");
  g_main_loop_unref(loop);
  loop = NULL;
  cleanup(0);
}

////////////////////////////////////////////////////
// CRC handling
////////////////////////////////////////////////////
static const uint16_t crc16Table[256]=
{
  0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
  0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
  0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
  0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
  0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
  0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
  0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
  0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
  0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
  0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
  0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
  0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
  0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
  0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
  0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
  0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
  0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
  0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
  0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
  0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
  0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
  0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
  0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
  0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
  0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
  0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
  0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
  0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
  0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
  0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
  0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
  0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};


/**
 * add new the CRC
 *
 * @param ptr : new character
 * @param CRC : previous CRC
 *
 * return
 * New CRC
 */
unsigned short add_crc(char ptr, unsigned short crc)
{
  return (crc << 8) ^ crc16Table[((crc >> 8) ^ ptr) & 0xff];
}

/**
 * Compute the CRC
 *
 * @param p    : pointer to buffer
 * @param lenb : length of buffer
 *
 * return : CRC on buffer
 */

uint16_t calc_crc(uint8_t *p, uint16_t lenb)
{
  uint16_t crc = 0;
  uint16_t i = 0;

  // use the complete previous block
  for (i=0; i< lenb ; i++){
    crc = add_crc(*p++, crc);
  }
  return(crc);
}

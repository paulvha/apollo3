/* This is a peripheral for the AMDTP / throughput monitor

  It works in conjunction with the peripheral MBED-BLE_example17_gattclient_AMDTP_troughput or 
  the BLEAK througput example

  This is a example of using AMD Transfer Protocol (AMDTP) service. With AMDTP you can send larger
  data sizes (by default up to 512 bytes). This data will be broken down by the sender in smaller 
  packages to the size of the MTU. The smaller pacakages will be controlled exchanged between sender and 
  receiver with the use of the AMDTP. On the receiver side these package will be combined 
  into a single data package and the end-to-end CRC will be checked to make sure the data is correct.

  In this example we exchange only a 512 data package to the client and he client will echo back
  the same package.  

  The AMDTP is based on the source code that is provided with AmbiqSuiteSDK. More information is available
  in the extras folder of the library.

  The sketch and connection to the server / peripheral is partly based on
  https://os.mbed.com/teams/mbed-os-examples/code/mbed-os-example-ble-GattClient/file/71d7cec222eb/main.cpp/
  
  But has many more features added

  Version 1.0 / February 2022 / paulvha
  - initial release


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
 * 
 * some background.
 *
 * The AMD transfer Protocol (AMDTP) allows sending larger datapackets between
 * client/central and server/peripheral. The default is set to max 512 bytes, which
 * will do for most sensor value exchange.
 *
 * The larger datapacket is broken down in smaller packages based on the agreed MTU size.
 * By default the MTU size starts at 23 bytes. After removing the 3 bytes overhead it means
 * 20 bytes per sendingpacket (unless larger MTU size was agreed)
 *
 * On the receiving side the small chunks are combined again into a single large datapacket,
 * including an CRC-32 control to make sure we have the correct data received.
 *
 * The sending from server/peripheral is happening on a notify characteristic. We can send
 * very quickly on that, but the client/central side it needs to be able to keep up with
 * that speed with enough buffers. That is often NOT the case if you send 26 packet ( 512 /20)
 * very fast.
 *
 * Hence it is implemented that after each 20 byte (or MTU size) packet has been sent the
 * sending side (either client/central or server/peripheral) will wait for the other side to
 * confirm it has received this packet. Only after that it will send the next chunk / packet.
 *
 * The result is that all the packets are received, BUT each packet will take a turn-around time
 * of 375ms. Measured between Artemis Edge and Artemis ATP board.
 * Thus sending 512 bytes will take 10s on an a default MTU size.
 * 
 * When connecting to BLEAK (on Ubuntu) the 512 bytes takes 3 seconds. MUCH faster, but still
 * 3000 / 512 = 6ms per byte = 1000 / 6 = 160 bytes per second.
 *
 * Not fast... but for many sensor this speed  will be acceptable as most produce only a small
 * amount of bytes
 *
 *
 *****************************************************************************************/

///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "amdtps.h"

// Input check in milliseconds
static const std::chrono::milliseconds CHECK_RATE_MS = 450ms;

// name of device
// this can be used on the Artemis client to find and connect.
// on the bleak client the MAC address needs to be used.
char _device[20]= "MBED_Throughput";

/*  On the client side a 500mS wait had to be included to allow the ACK to be send
 *  on the earlier received data before start echo the data back. Else the first packet 
 *  seems to be lost.
 */
unsigned long CLIENTWAIT = 500;

//////////////////////////////////////////////////////////////////////////////////////////
// NO CHANGE NEEDED BEYOND THIS POINT
/////////////////////////////////////////////////////////////////////////////////////////

// event handler
static events::EventQueue event_queue(/* event count */ 16 * EVENTS_EVENT_SIZE);

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

// the Gattserver constructor
GattServAMDTP AMDSvc(ble_instance, event_queue);

unsigned long StartTime = 0;       // determine start sending time
unsigned long ReceiveTime = 0;     // determine time when response received

#define BUFLEN 512
uint8_t SendBuf[BUFLEN];           // store data to send
uint16_t SendBufLen = 0 ;          // length of data

uint8_t ReceiveBuf[BUFLEN];        // store central received data
uint16_t ReceiveBufLen = 0 ;       // store length of received data
bool DataReceived = false;         // indicate that data was received from Central

void setup() {
  
  Serial.begin(115200);
  while (!Serial) { delay(500); }

  flush();
  Serial.println("\r\nExample17 : Gatt Server AMDTP Throughput test\r\nPress Enter to start or wait 2 seconds\r");
  for(uint8_t i = 0 ; i < 20; i++) {
    if (Serial.available()) break;
    delay(100);
  }

  flush();

#ifdef BLE_Debug
  Enable_BLE_Debug(true);
#endif

  Fill_buffer();

  // set the advertise device name
  AMDSvc.set_device_name(_device);
 
  // set handler to call to process events
  ble_instance.onEventsToProcess(schedule_ble_events);

  // add loop to check as a recurring event on the event queue
  event_queue.call_every(CHECK_RATE_MS, &Check_loop);

  // initialise the AMD transfer protocol service
  AMDSvc.start();
}

void loop() {
  // put your main code here, to run repeatedly:
  // moved to Check_loop()
}

#define NUMPACK 14
// fill buffer with 512 characters
void Fill_buffer()
{
  uint8_t a,b;
  
  for (a = 0; a < NUMPACK ; a++) {
    
    for(b = 'a'; b <= 'z'; b++)       // add a to z = 26
      SendBuf[SendBufLen++] = b;
     

    for(b = '0'; b <= '9'; b++)       // add 0 to 9 = 10
      SendBuf[SendBufLen++] = b;
    
  }

  for(b = '0'; b <= '6'; b++)         // add 0 to 6 = 7
      SendBuf[SendBufLen++] = b;
  
  SendBuf[SendBufLen++] = 0x0a;
  SendBuf[SendBufLen]   = 0x00;
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
  // needed for headers
  static bool setq = true;
  static bool once = true;
  static bool onceA = true;
  static bool WaitSend = false;

  if (onceA) {
    
    if (AMDSvc.IsAdvertising()) {
      printf("Connect to %s with ", _device);
      print_mac_address();
      onceA = false;
    } 
  }
  
  if (AMDSvc.IsConnected()) {
   
    if (once){
      printf("Client connected, you may now subscribe to updates\r\n");
      once = false;
    }
   
    if (setq) {
      printf("Press enter to start sending\r\n");
      setq = false;
    }
  }
  else
  {
    // reset ( in case connection was lost, re-display headers)
    if (! once && ! setq) onceA = true;
    once = true;
    setq = true;
  }
  
  // handle any keyboard input
  if (Serial.available()) {
    flush();
    setq = true;

    if(! WaitSend) {
      
      StartTime = millis();
      
      int ret = AMDSvc.SendToCentral((uint8_t *) SendBuf, SendBufLen);

      switch(ret) {
        case -2 :
          printf("Not connected. ");
          // fall through
        case -1 :
          printf("Error during sending\r\n");
          break;
        case 0:
          printf("Send command took %d ms, num bytes %d\r\n", millis() - StartTime, SendBufLen);
          break;
        case 1:
          printf("Sending in different packages. Wait to complete\r\n");
          WaitSend = true;
          setq = false;
          break;
      }
    }
    else
    {
      printf("Can not send. Previous package is not complete\r\n");
    }
  }

  // waiting for previous package to complete 
  if (WaitSend){
    
    if (AMDSvc.IsSendingComplete()) {
      printf("Send command took %d ms, num bytes %d\r\n", millis() - StartTime, SendBufLen);
      WaitSend = false;
    }
  }
         
  // handle central command received
  if (DataReceived) {
    setq = true;
    HandleDataReceived();
  }
}

/* take action on received command from central/client */
void HandleDataReceived()
{
/*
  // debug only
  printf("len = %d\r\n", SendBufLen);
  for (uint8_t i; i < SendBufLen ; i++) printf("%x %d\r\n",SendBuf[i], SendBuf[i]);
*/

  printf("Turn around took %d ms\r\n", ReceiveTime - StartTime - CLIENTWAIT);

  // calculate throughput
  DataReceived = false;
}

/* handoff received data from central / client */
void StoreDataReceived(uint8_t *data, uint16_t len)
{
  ReceiveTime = millis();
  
  if (len > 0) {
    SendBufLen  = len;
    if (ReceiveBufLen > BUFLEN)  ReceiveBufLen = BUFLEN;
    for (uint8_t i = 0; i < ReceiveBufLen ; i++) ReceiveBuf[i] = data[i];
    DataReceived = true;    // indicate we have new data received
  }
}

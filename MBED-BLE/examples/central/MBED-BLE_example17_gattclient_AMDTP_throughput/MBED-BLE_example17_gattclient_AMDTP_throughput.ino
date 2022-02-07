/*
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
 
  # BLE Gatt Client example
   
  This application demonstrates detailed uses of the GattClient APIs. 

  It works in conjunction with the peripheral MBED-BLE_example17_gattserv_AMDTP_troughput

  This is a example of using AMD Transfer Protocol (AMDTP) service. With AMDTP you can send larger
  data sizes (by default up to 512 bytes). This data will be broken down by the sender in smaller 
  packages to the size of the MTU. The smaller pacakages will be controlled exchanged between sender and 
  receiver with the use of the AMDTP. On the receiver side these package will be combined 
  into a single data package and the end-to-end CRC will be checked to make sure the data is correct.
  
  The sketch and connection to the server / peripheral is partly based on
  https://os.mbed.com/teams/mbed-os-examples/code/mbed-os-example-ble-GattClient/file/71d7cec222eb/main.cpp/
  
  But has many more features added

  Version 1.0 / January 2022 / paulvha

 ************************************************************************************************** 
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

#include "gatt_client_process.h"

// event handler
static events::EventQueue event_queue(16 * EVENTS_EVENT_SIZE);

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

/* this process will handle basic ble setup and advertising, connecting for us */
GattClientProcess ble_process(event_queue, ble_instance);

// Input check in milliseconds
static const std::chrono::milliseconds CHECK_RATE_MS = 450ms;

// name of peer device to look for
char peer_device[20]= "MBED_Throughput";

unsigned long StartTime = 0;       // determine start sending time
unsigned long ReceiveTime = 0;     // determine time when response received

#define BUFLEN 512
uint8_t ReceiveBuf[BUFLEN];        // store central received data
uint16_t ReceiveBufLen = 0 ;       // store length of received data
bool DataReceived = false;         // indicate that data was received from Central
bool Sending_in_chunks = false;    // indicate that sending in chunks is in progress
///////////////////////////////////////////////////////////
// NO CHANGES NEEDED BEYOND THIS POINT
///////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(500); }
  Serial.println("\r\nExample17 Gatt Client AMDTP Throughput\r");

#ifdef BLE_Debug
    Enable_BLE_Debug(true);
#endif

  // set name of peripheral to look for
  ble_process.set_peer_device_name(peer_device); 
  
  // add keyboard check as a recurring event on the event queue
  event_queue.call_every(CHECK_RATE_MS, &Check_loop);    

  printf("Start search for device %s\r\n", peer_device);

  ble_process.start();
}

void loop() {
  // put your main code here, to run repeatedly:
  // NOT USED. USE Check_Loop()
}

/* called regular from event queue as define in setup() */
void Check_loop()
{
  static bool ShowOnce = true;

  // connected, all characteristic discovered
  if (ble_process.ConnectionComplete()) {
    
    // only show menu after discovery is complete
    if (ShowOnce){
      printf("Discovery completed. Wait for data\r\n");
      ShowOnce = false;
    }
  }
  else
    ShowOnce = true;

  // handle received data
  if (DataReceived) {
    // wait 500 ms before sending  
    if (millis() - ReceiveTime > 500){
      SendEcho();
      DataReceived = false; 
    }
  }

  // make sure we are still connected
  if (! ble_process.IsConnected()) {
    ShowOnce = true;
  }
  
  // if sending in chunk is in progress
  if (Sending_in_chunks) {
    if (ble_process.IsSendingComplete()){
      printf("All has been sent in %d ms\r\n", millis() - StartTime);
      Sending_in_chunks = false;
    }
  }
}

/* send the command to peripheral/server */
void SendEcho()
{
  StartTime = millis();
  
  int ret = ble_process.SendToServer(ReceiveBuf, ReceiveBufLen);
  if (ret == -1) {
    printf("Error during sending echo\r\n");
  }
  else if (ret == 1) {
    printf("Sending back in different packages. Wait to complete\r\n");
    Sending_in_chunks = true;
  }
  else
    printf("Send command took %d ms\r\n", millis() - StartTime);
}

/* here is the data is being received handled */
void StoreDataReceived(uint8_t *data, uint16_t len){
  
  ReceiveTime = millis(); 
  if (len > 0){
    ReceiveBufLen  = len;
    if (ReceiveBufLen > BUFLEN)  ReceiveBufLen = BUFLEN;
    for (uint16_t i = 0; i < ReceiveBufLen ; i++) ReceiveBuf[i] = data[i];
    DataReceived = true;    // indicate we have new data received
  }
  printf("received message length %d\r\n", len);
}

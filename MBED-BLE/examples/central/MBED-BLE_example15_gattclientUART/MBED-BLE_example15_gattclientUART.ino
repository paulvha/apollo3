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

    It works in conjunction with the peripheral MBED-BLE_example15_gattservUART
     
    When the application is started looking for a device called MBED_UartART and when
    discovered this application reads the value of the characteristics 
    discovered and subscribes to the characteristic 6E400002-B5A3-F393-E0A9-E50E24DCCA9E for
    emitting notifications or indications. 

    It will wait for data to be send by the peripheral. But next to that the client
    can send feedavack to the server.

    It is partly based on
    based on https://github.com/desmond-blue/mbed-ble-uart
    
    But has many more features added

    Version 1.0 / January 2022 / paulvha
 */
///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "gatt_client_process.h"

String BLE_UUID_NUS_SERVICE = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
String BLE_UUID_NUS_TX_CHARACTERISTIC = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";   // for NUS server,RX for client
String BLE_UUID_NUS_RX_CHARACTERISTIC = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";   // for NUS server, TX for client

// event handler
static events::EventQueue event_queue(16 * EVENTS_EVENT_SIZE);

// the Gattdemo constructor
GattClientDemo demo;

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

/* this process will handle basic ble setup and advertising for us */
GattClientProcess ble_process(event_queue, ble_instance);

// Input check in milliseconds
static const std::chrono::milliseconds CHECK_RATE_MS = 450ms;

// name of peer device to look for
char peer_device[20]= "BLE_Uart";

static const unsigned  BUFLEN  = (BLE_GATT_MTU_SIZE_DEFAULT - 3);
uint8_t  input[BUFLEN];             // keyboard input buffer
int      inpcnt = 0;                // keyboard input buffer length

///////////////////////////////////////////////////////////
// NO CHANGES NEEDED BEYOND THIS POINT
///////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(500); }
  Serial.println("\r\nExample15 GattClient UART\r");

#ifdef BLE_Debug
    Enable_BLE_Debug(true);
#endif
    
  /* once it's done it will let us continue with our demo */
  ble_process.on_init(mbed::callback(&demo, &GattClientDemo::start));
  ble_process.on_connect(mbed::callback(&demo, &GattClientDemo::start_discovery));
  ble_process.on_disconnect(mbed::callback(&demo, &GattClientDemo::stop));

  // set name of peripheral to look for
  ble_process.set_peer_device_name(peer_device);

  // (optional) add services filter
  // MAXUUID (defined in gattclient.h) can be added (default 5)
  // Set service like: "19B10000-E8F2-537E-4F6C-D104768A1214" or "180F" 
  // NO '0x' in front
  SetFilter(BLE_UUID_NUS_SERVICE);

  // determine which characteristic to use to send to peripheral
  SetTXChar(BLE_UUID_NUS_RX_CHARACTERISTIC);
  
  // add keyboard check as a recurring event on the event queue
  event_queue.call_every(CHECK_RATE_MS, &Check_loop);    

  printf("Start search for device %s\r\n", peer_device);
  
  ble_process.start();
}

void loop() {
  // put your main code here, to run repeatedly:
  // NOT USED. USE Check_Loop()
}

/* called regular from event queue */
void Check_loop()
{
  static bool ShowOnce = true;
  
  // only show menu after after discovery is complete
  if (ShowOnce){
    // if discover is complete
    if (demo.ConnectionComplete()) {
      printf("Discovery completed. you can now type a message.\r\n");
      ShowOnce = false;
    }
  }
  
  // check & handle any keyboard input
  if (Serial.available()) {
    while (Serial.available()) {
      handle_input(Serial.read());
      delay(50);
    }
  }
}

/* handle local keyboard input */
void handle_input(char c)
{
  if (c == '\r') return;    // skip CR
        
  if (c != '\n') {          // act on linefeed
    input[inpcnt++] = c;
    if (inpcnt < BUFLEN -1 ) return; // prefend overflow
  }
  
  input[inpcnt] = 0x0;

  SendInput(input, inpcnt);
  
  // reset keyboard buffer
  inpcnt = 0;
}

/* send to the peripheral/server */
void SendInput(uint8_t *inp, uint8_t len)
{
  if (! demo.WriteTXChar(inp, len)) {
    printf("Error during writing command\r\n");
  }
}

/* here is the UART data from server is being received handled */
void On_data_received(const GattHVXCallbackParams* event){
/*
   // debug only
   printf("Data received %u: new value = ", event->handle);
    for (size_t i = 0; i < event->len; ++i) {
        printf("0x%02X ", event->data[i]);
    }
    printf("\r\n");
*/
    printf("Data received %u: new value = ", event->handle);
    for (size_t i = 0; i < event->len; ++i) {
        printf("%c", event->data[i]);
    }
    printf("\r\n");
}

/* enable Service filter */
void SetFilter(String sfilter)
{
  uint8_t filter[16];
  int k;
  
  // set UUID filter  
  if (sfilter.length() > 0) {
    k = StringToHex(sfilter, &filter[0]);
    if (k > 0) demo.AddServiceFilter(filter, k);
  }
}

/* set the characteristic UUID for sending to server/peripheral */
void SetTXChar(String TXChar)
{
  uint8_t filter[16];
  int k;

  // set UUID filter  
  if (TXChar.length() > 0) {
    k = StringToHex(TXChar, &filter[0]);
    if (k > 0) demo.AddTXChar(filter, k);
  }
}

/** parse a string to hex bytes in reverse order
 *  
 *  @param input : string to parse
 *  @param output : array to store
 *  
 *  return: 
 *    length in bytes
 *    0 = error
 */
int StringToHex(String input, uint8_t *output)
{
  uint8_t i, cnt, h, t;
  uint8_t l = input.length();
  int ret;
  bool Nib_st = true;

  // determine length of input
  if (l == 17) ret = 6;      // address       
  else if (l > 32) ret = 16; // UUID 128 bits
  else if (l > 4) ret = 4;   // UUID 32 bits
  else ret = 2;              // UUID 16 bits
 
  for(i = 0, cnt = 0; i < l; i++) {
    
    h = input[i];
    
    if (h == '-' || h == ':') continue;

    // ascii to hex
    if( h >= '0' && h <= '9') h = h - '0';
    else if( h >= 'a' && h <= 'f') h = h - 'a' + 0xa;
    else h = h - 'A' + 0xa;

    // store in single byte
    if (Nib_st) {
      t = (h & 0xf) << 4; 
      Nib_st = !Nib_st;
    }
    else {
      t |= (h & 0xf); 
      Nib_st = !Nib_st;

      output[cnt++] = t;
    }
  }
   
  if (! Nib_st ) {
    printf("Error during translating \r\n");
    Serial.println(input);
    ret = 0;
  }

  return(ret);
}

/*
 * 
 *  * Licensed under the Apache License, Version 2.0 (the "License");
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
 
    # BLE Gatt heartrate example
     
    This application demonstrates uses of the GattClient APIs. If will connect
    with the peripheral/server sketch MBED_BLE_example13_gattservHR which is providing
    a simulation of heartrate

    Parts are based on
    https://os.mbed.com/teams/mbed-os-examples/code/mbed-os-example-ble-GattClient/file/71d7cec222eb/main.cpp/

    But has many more features added

    Version 1.0 / January 2022 / paulvha
 */
///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "gatt_client_process.h"

// event handler
static events::EventQueue event_queue(16 * EVENTS_EVENT_SIZE);

// the Gattdemo constructor
GattClientDemo demo;

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

/* this process will handle basic ble setup and advertising for us */
GattClientProcess ble_process(event_queue, ble_instance);

// name of peer device to look for
char peer_device[20]= "Heartrate";

///////////////////////////////////////////////////////////
// NO CHANGES NEEDED BEYOND THIS POINT
///////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(500); }
  Serial.println("\r\nGattClient example13 Heartrate\r");

#ifdef BLE_Debug
    Enable_BLE_Debug(true);
#endif
    
    /* once it's done it will let us continue with our demo */
    ble_process.on_init(mbed::callback(&demo, &GattClientDemo::start));
    ble_process.on_connect(mbed::callback(&demo, &GattClientDemo::start_discovery));

    // set name of peripheral to look for
    ble_process.set_peer_device_name(peer_device);

    // (optional) add services filter
    // MAXUUID (defined in gattclient.h) can be added (default 5)
    // Set service like: "19B10000-E8F2-537E-4F6C-D104768A1214" or "180F" 
    // NO '0x' in front
    SetFilter(String ("1801"));
    SetFilter(String ("180D"));
    
    ble_process.start();
}

void loop() {
  // put your main code here, to run repeatedly:

}

// here is the data being received handled
void On_data_received(const GattHVXCallbackParams* event){
/*   
   printf("Data received %u: new value = ", event->handle);
    for (size_t i = 0; i < event->len; ++i) {
        printf("0x%02X ", event->data[i]);
    }
    printf("\r\n");
*/
    uint16_t HeartRate = event->data[0] << 8 | event->data[1];

    printf("received heartrate %d\r\n", HeartRate);
}


/** enable Service filter if defined */
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

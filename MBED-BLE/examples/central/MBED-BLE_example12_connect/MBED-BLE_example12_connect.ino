/*
 * Scanner for BLE on Mbed/Cordio
 *
 * This example will scan for other Bluetooth Devices for a certain
 * amount of time and will display for each unique detected device
 * the scan information.
 *
 * It will then ask whether you want to connect. You can also set
 * a device address and/or service UUID and /or device name.
 * It will not ask and if either matches it will connect.
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
 *
 * The INO has has been developed by Version 1.0 / January 2022 / paulvha
 */

///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "ble_ScanConnect.h"

/** filter options
 *  if match (either UUID or address) it will connect automatically
 */

/* set UUID filter. Max 16 bytes (128 bits), 4 bytes (32 bits) and 2 bytes(16 bits) can be set
 * The length is automatically detected
 * Set service like: "19B10000-E8F2-537E-4F6C-D104768A1214" or "180F"
 * NO '0x' in front
 * To disable define as Filter_Service=""
 */
//String Filter_Service="19B10000-E8F2-537E-4F6C-D104768A1214";
String Filter_Service="";   // NO UUID filter

/* set address as it is displayed (do not reverse)
 * To disable define as Filter_BdAddr=""
 */
//String Filter_BdAddr="64:D2:C4:B5:C3:B9";
String Filter_BdAddr="";    // No address filter

/* set peer device name filter.
 * To disable define as Filter_Service=""
 */
//String Filter_Name="led";
String Filter_Name="";  // NO name filter

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

// event handler
static events::EventQueue event_queue(16 * EVENTS_EVENT_SIZE);

// scanner constructor
Ble_scan_connect scanner(ble_instance, event_queue);

void setup() {

  Serial.begin(115200);
  while (!Serial) { delay(500); }
  Serial.println("\r\nExample12 : Scanner & connect & filter");

#ifdef BLE_Debug
  Enable_BLE_Debug(true);
#endif

  // check for filters
  SetFilters();

  // set handler to call to process events
  ble_instance.onEventsToProcess(schedule_ble_events);

  // start scanner
  scanner.run();
}

/* although we do not use loop(), we have to keep this in place else compile will fail */
void loop() {}


/** enable filters if defined */
void SetFilters()
{
  uint8_t filter[FILTERSIZE];
  int k;

  // check for address filter
  if (Filter_BdAddr.length() > 0) {
    k = StringToHex(Filter_BdAddr, &filter[0]);
    if (k > 0) scanner.SetAdrFilter(filter);
  }

  // check for UUID filter
  if (Filter_Service.length() > 0) {
    k = StringToHex(Filter_Service, &filter[0]);
    if (k > 0) scanner.SetUuidFilter(filter, k);
  }

  // check for Name filter
  if (Filter_Name.length() > 0) {
    scanner.SetNameFilter(Filter_Name);
  }
}

/* Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
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
  int pos, ret;

  // determine length of input
  if (l == 17) pos = 6;      // address
  else if (l > 32) pos = 16; // UUID 128 bits
  else if (l > 4) pos = 4;   // UUID 32 bits
  else pos = 2;              // UUID 16 bits

  ret = pos;

  pos--;                      // end at pos zero

  for(i = 0, cnt = 0; i < l; i++) {

    h = input[i];

    if (h == '-' || h == ':') continue;

    // ascii to hex
    if( h >= '0' && h <= '9') h = h - '0';
    else if( h >= 'a' && h <= 'f') h = h - 'a' + 0xa;
    else h = h - 'A' + 0xa;

    // store in single byte
    if (cnt == 0) {
      t = (h & 0xf) << 4;
      cnt=1;
    }
    else {
      t |= (h & 0xf);
      cnt=0;

      // store in reverse order
      output[pos--] = t;
    }
  }

  if (cnt != 0 ) {
    printf("Error during translating \r\n");
    Serial.println(input);
    ret = 0;
  }

  return(ret);
}

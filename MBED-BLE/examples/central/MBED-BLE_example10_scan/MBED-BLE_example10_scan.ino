/*
 * Scanner for BLE on Mbed/Cordio
 *
 * This example will scan for other Bluetooth Devices for a certain
 * amount of time and will display for each unique detected device
 * the scan information.
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
 * The sketch has has been developed by paulvha
 * Version 1.0 / January 2021
 */
 ///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "BLEscanner.h"

static events::EventQueue event_queue(4 * EVENTS_EVENT_SIZE);

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

// scanner constructor
Ble_scan scanner(ble_instance, event_queue);

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(500); }
  Serial.println("\r\nExample10 :MBED BLE Scanner for unique devices\r");

#ifdef BLE_Debug
  Enable_BLE_Debug(true);
#endif

  // set handler to call to process events
  ble_instance.onEventsToProcess(schedule_ble_events);

  // start demo
  scanner.run();
}

// can not be used.. see other examples how to set alternative
void loop() {

}

/* Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

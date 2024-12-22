/* mbed Microcontroller Library
 * Copyright (c) 2006-2019 ARM Limited
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
 * Based on example for BLE GAP
 * https://github.com/ARMmbed/mbed-os-example-ble/blob/master/BLE_GAP/source/main.cpp\
 *
 *
 */
/** This example demonstrates all the basic setup required to advertise or scan.
 *
 *  It contains a single class that performs BOTH scans and advertisements.
 *
 *  The demonstrations happens in sequence, after each "mode" ends
 *  the demo jumps to the next mode to continue.
 *
 *  You may connect to the device during advertising and if you advertise
 *  this demo will try to connect during the scanning phase.
 *
 *  Connection will terminate the phase early. At the end of the phase some stats
 *  will be shown about the phase.
 *
 * Version 1.0 / paulvha / January 2022
 */

///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "gapdemo.h"

// BLE constructor
BLE& ble_instance = ble_instance.Instance();  // BLE constructor

// event handler
static events::EventQueue event_queue(4 * EVENTS_EVENT_SIZE);

GapDemo demo(ble_instance, event_queue);

////////////////////////////////////////////////////////////
//  NO CHANGE NEEDED BEYOND THIS POINT                    //
////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);
  while (!Serial) { delay(500); }
  Serial.println("Test_example1  : Demo GAP scan & advertise");

#ifdef BLE_Debug
  Enable_BLE_Debug(true);
#endif

  // set handler to call to process events
  ble_instance.onEventsToProcess(schedule_ble_events);

  // start demo
  demo.run();
}

// can not be used...see other examples
void loop() {
  // put your main code here, to run repeatedly:

}

/* Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

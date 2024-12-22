/*
 * Scanner for BLE on Mbed/Cordio
 *
 * This example will scan for other Bluetooth Devices for a certain
 * amount of time and will display for each unique detected device
 * the scan information. It will also call regular a routine, which
 * for now only blinks the led but can be used instead of loop()
 *
 *
 * based on :
 * https://6point6.co.uk/insights/getting-started-with-bluetooth-le-development/
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
 * This is an extended version of example10 which does scanning only to  include
 * a timeout,  where it calls regular additional routine Do_Action() to blink
 * or (optional) other activities
 *
 * version 1.0 / January 2022 / paulvha
 */
///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "BLEscanner.h"

// event handler
static events::EventQueue event_queue(4 * EVENTS_EVENT_SIZE);

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

// scanner constructor
Ble_scan scanner(ble_instance, event_queue);

// LED blinking rate in milliseconds
static const std::chrono::milliseconds BLINKING_RATE_MS = 500ms;

void setup() {

  Serial.begin(115200);
  while (!Serial) { delay(500); }
  Serial.println("\r\nExample11 : MBED BLE Scanner & blink ");

#ifdef BLE_Debug
  Enable_BLE_Debug(true);
#endif

  // set handler to call to process events
  ble_instance.onEventsToProcess(schedule_ble_events);

  // add the LED blinker as a recurring event on the event queue
  event_queue.call_every(BLINKING_RATE_MS, &Do_Action);

  // enable led
  pinMode(LED_BUILTIN, OUTPUT);

  // start scanner
  scanner.run();
}

/* Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

/* although we do not use loop(), we have to keep this in place else compile will fail
 * instead put this in Do_Action() below */
void loop() {}

/** As an example here extra actions (like reading a sensor) could be performed */
void Do_Action() {

  // Led blink status
  static bool LedStatus = false;

  digitalWrite(LED_BUILTIN, LedStatus);
  LedStatus = !LedStatus;

  // put your main code here, to run repeatedly:
  // printf("BLINK : Here I can do an action\r\n");
}

/* This is a peripheral simulation of the heartrate monitor
 *
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
 *
 *usage
 * THis will create a simulate Heartrate sensor peripheral /server. can be used
 * injunction with MBED-BLE_example13_gattclientHR sketch or the Python_bleak_HR program
 * that can run on windows or linux/ubuntu
 * 
 * Adjusted for Sparkfun Artemis / version 1.0 / paulvha / January  2022
 */

///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "gatt_server_HR.h"

// event handler
static events::EventQueue event_queue(/* event count */ 16 * EVENTS_EVENT_SIZE);

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

// the Gattserver Heartrate constructor
GattServHR HR(ble_instance, event_queue);

// name of device
char _device[20]= "Heartrate";

// variable to send (defined in gatt_server_HR.h)
extern uint16_t _heartrate_value;

///////////////////////////////////////////////////////////
// NO CHANGES NEEDED BEYOND THIS POINT
///////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(500); }

  flush();
  Serial.println("\r\nGatt Server Heartrate example13\r\nPress Enter to start or wait 2 seconds\r");
  for(uint8_t i =0; i < 20; i++) {
    if (Serial.available()) break;
    delay(100);
  };
  flush();

#ifdef BLE_Debug
  Enable_BLE_Debug(true);
#endif

  // set initial value
  _heartrate_value=80;

  // set the advertise device name
  HR.set_device_name(_device);

  // set handler to call to process events
  ble_instance.onEventsToProcess(schedule_ble_events);

  HR.start();
}

void loop() {
  // put your main code here, to run repeatedly:
  // moved to update_value()

}

/*
 * flush any pending input
 */
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

/*
 * Here we update the heart rate value
 * this can be replaced with a updates of other sensors.
 */
void update_value()
{
    /* you can read in the real value but here we just simulate a value */
    _heartrate_value++;

    /*  60 <= bpm value < 110 */
    if (_heartrate_value == 110) {
        _heartrate_value = 60;
    }
    Serial.print("update ");
    Serial.println(_heartrate_value, HEX);
}

/* mbed Microcontroller Library
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
 * based on https://github.com/desmond-blue/mbed-ble-uart
 * 
 * parts added paulvha
 * 
 * Version 1.0 / January 2022
 */

///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "uart.h"  

// LED blinking rate in milliseconds
static const std::chrono::milliseconds BLINKING_RATE_MS = 500ms;

//////////////////////////////////////////////////////////////////////////////////////////
// NO CHANGE NEEDED BEYOND THIS POINT
/////////////////////////////////////////////////////////////////////////////////////////

static events::EventQueue event_queue(/* event count */ 16 * EVENTS_EVENT_SIZE);

BLE &ble_instance = BLE::Instance();

BleUartDemo demo(ble_instance, event_queue);  

static const unsigned  BUFLEN  = (BLE_GATT_MTU_SIZE_DEFAULT - 3);
uint8_t  input[BUFLEN];             // keyboard input buffer
int      inpcnt = 0;                // keyboard input buffer length

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(500); }

  flush();
  Serial.println("\r\nExample16 : Gatt Server NUS\r\nPress Enter to start or wait 2 seconds\r");
  for(uint8_t i =0; i < 20; i++) {
    if (Serial.available()) break;
    delay(100);
  }

  flush();

#ifdef BLE_Debug
  Enable_BLE_Debug(true);
#endif

  ble_instance.onEventsToProcess(schedule_ble_events);

  // add the LED blinker as a recurring event on the event queue
  event_queue.call_every(BLINKING_RATE_MS, &Do_Action);
  
  // enable the led
  pinMode(LED_BUILTIN, OUTPUT);
  
  demo.start();
}

void loop() {
  // put your main code here, to run repeatedly:
  // not used... look at Do_action()

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

/** As an example here extra actions (like reading a sensor) could be performed */
void Do_Action() {

  // Led blink status
  static bool LedStatus = false;

  digitalWrite(LED_BUILTIN, LedStatus);
  LedStatus = !LedStatus;

  // check & handle any keyboard input
  if (Serial.available()) {
    while (Serial.available()) {
      handle_input(Serial.read());
      delay(50);
    }
  }
}

/* handle local keyboard input */
void handle_input(char c){
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
  if (! demo.SetMessage(inp, len)) {
    printf("Error during writing command\r\n");
  }
}

/* here is the UART data from client is being received handled */
void On_data_received(const GattWriteCallbackParams* event){

   // debug only
   printf("Data received %u: new value = ", event->handle);
   for (size_t i = 0; i < event->len; ++i) {
        printf("0x%02X ", event->data[i]);
   }
   printf("\r\n");

   for (size_t i = 0; i < event->len; ++i) {
        printf("%c", event->data[i]);
   }
   printf("\r\n");
    
}

/** Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context) {
    event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

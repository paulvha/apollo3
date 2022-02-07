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

    # BLE Gatt Client example

    This application demonstrates detailed uses of the GattClient APIs.

    When the application is started it advertises itself to its environment with the
    device name `GattClient`. Once you have connected to the device with your mobile
    phone, the application starts a discovery of the GATT server exposed by your
    mobile phone.

    After the discovery, this application reads the value of the characteristics
    discovered and subscribes to the characteristics emitting notifications or
    indications.

    The device prints the value of any indication or notification received from the
    mobile phone.

    After 4 seconds it will stop advertising and start scanning. It will look for a
    device with specific name. If detected it will connected to the that device and
    Once you have connected to the device, the application starts a discovery of the
    services, characterisc and descriptors on that pheripheral.

    It is based on
    https://os.mbed.com/teams/mbed-os-examples/code/mbed-os-example-ble-GattClient/file/71d7cec222eb/main.cpp/

    But has many more features added

    Version 1.0 / January 2022 / paulvha
 */
///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "gattclient.h"

// the Gattdemo constructor
GattClientDemo demo;

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

// event handler
static events::EventQueue event_queue(16 * EVENTS_EVENT_SIZE);

/* this process will handle basic ble setup and advertising for us */
GattClientProcess ble_process(event_queue, ble_instance);

//name of peer device to look for
char peer_device[20]= "BATTERY";

/////////////////////////////////////////////////////////////////
// No changes needed beyond this point
/////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(500); }
  Serial.println("\r\nTest_example2 : GattClient\r");

#ifdef BLE_Debug
    EnableCordioDebug(true);
    WsfTraceRegisterHandler(( WsfTraceHandler_t) traceCback);
    WsfTraceEnable(true);
#endif

    ble_process.set_peer_device_name(peer_device);

    /* once it's done it will let us continue with our demo */
    ble_process.on_init(mbed::callback(&demo, &GattClientDemo::start));
    ble_process.on_connect(mbed::callback(&demo, &GattClientDemo::start_discovery));

    ble_process.start();
}

void loop() {
  // put your main code here, to run repeatedly:

}

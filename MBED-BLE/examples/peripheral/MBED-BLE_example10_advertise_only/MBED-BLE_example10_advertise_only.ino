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
 * Based on example for BLE advertising
 * https://github.com/ARMmbed/mbed-os-example-ble/blob/master/BLE_Advertising/source/main.cpp
 *
 * USAGE
 * The example will advertise a simulated battery level only. You can not
 * connect, but see the battery level with every scan. The battery
 * level is updated every second.
 *
 * paulvha / January 2022 / version 1.0
 */
///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "mbed_ble.h"

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
ble::AdvertisingDataBuilder _adv_data_builder(_adv_buffer,ble::LEGACY_ADVERTISING_MAX_SIZE) ;

// event handler
static events::EventQueue event_queue(/* event count */ 4 * EVENTS_EVENT_SIZE);

// program variables to update
uint8_t battery_level = 50;
const static char DEVICE_NAME[] = "BATTERY";

////////////////////////////////////////////////////////////
//  NO CHANGE NEEDED BEYOND THIS POINT                    //
////////////////////////////////////////////////////////////
void setup() {

  Serial.begin(115200);

  while (!Serial) { delay(500); }

  Serial.println("Example 10: Demo of simulated battery");

#ifdef BLE_Debug
  Enable_BLE_Debug(true);
#endif

  // set handler to call to process events
  ble_instance.onEventsToProcess(schedule_ble_events);

  // mbed will call on_init_complete when ble is ready
  ble_instance.init(on_init_complete);

  // Launch BLE initialisation & Dispatch events in the event queue
  event_queue.dispatch_forever();

}

void loop() {
  // put your main code here, to run repeatedly:
  // NOT USED

}

/* Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
  event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

/* called after BLE initialisation is complete */
void on_init_complete(BLE::InitializationCompleteCallbackContext *params)
{
  if (params->error != BLE_ERROR_NONE) {
    Serial.println("BLE initialization failed.");
    while(1);
  }

  Serial.println("BLE initialization complete.");

  start_advertising();
}

void start_advertising()
{
  /* create advertising parameters and payload */

  ble::AdvertisingParameters adv_parameters(
    /* you cannot connect to this device, you can only read its advertising data,
     * scannable means that the device has extra advertising data that the peer can receive if it
     * "scans" it which means it is using active scanning (it sends a scan request) */
    ble::advertising_type_t::SCANNABLE_UNDIRECTED,
    ble::adv_interval_t(ble::millisecond_t(1000))
  );

  /* when advertising you can optionally add extra data that is only sent
   * if the central requests it by doing active scanning (sending scan requests),
   * in this example we set this payload first because we want to later reuse
   * the same _adv_data_builder builder for payload updates */

  const uint8_t _vendor_specific_data[4] = { 0x50, 0x41, 0x55, 0x4C };
  _adv_data_builder.setManufacturerSpecificData(_vendor_specific_data);

  ble_instance.gap().setAdvertisingScanResponse(
    ble::LEGACY_ADVERTISING_HANDLE,
    _adv_data_builder.getAdvertisingData()
  );

  /* now we set the advertising payload that gets sent during advertising without any scan requests */

  _adv_data_builder.clear();
  _adv_data_builder.setFlags();
  _adv_data_builder.setName(DEVICE_NAME);

  /* we add the battery level as part of the payload so it's visible to any device that scans,
   * this part of the payload will be updated periodically without affecting the rest of the payload */
  _adv_data_builder.setServiceData(GattService::UUID_BATTERY_SERVICE, {&battery_level, 1});

  /* setup advertising */
  ble_error_t error = ble_instance.gap().setAdvertisingParameters(
    ble::LEGACY_ADVERTISING_HANDLE,
    adv_parameters
  );

  if (error) {
    print_error(error, "setAdvertisingParameters() failed. Freeze");
    while(1);
  }

  error = ble_instance.gap().setAdvertisingPayload(
    ble::LEGACY_ADVERTISING_HANDLE,
    _adv_data_builder.getAdvertisingData()
  );

  if (error) {
    print_error(error, "setAdvertisingPayload() failed. Freeze");
    while(1);
  }

  /* start advertising */
  error = ble_instance.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

  if (error) {
    print_error(error, "startAdvertising() failed. Freeze");
    while(1);
  }

  /* we simulate battery discharging by updating it every second */
  event_queue.call_every(1000ms, updatebattery_level );
}

// simulate a battery level update
void updatebattery_level()
{
  if (battery_level-- == 10) {
    battery_level = 100;
  }

  /* update the payload with the new value of the bettery level, the rest of the payload remains the same */
  ble_error_t error = _adv_data_builder.setServiceData(GattService::UUID_BATTERY_SERVICE, mbed::make_Span(&battery_level, 1));

  if (error) {
    print_error(error, "setServiceData() failed.");
    return;
  }

  /* set the new payload, we don't need to stop advertising */
  error = ble_instance.gap().setAdvertisingPayload(
      ble::LEGACY_ADVERTISING_HANDLE,
      _adv_data_builder.getAdvertisingData()
  );

  if (error) {
    print_error(error, "setAdvertisingPayload() failed.");
    return;
  }

  Serial.print("batterylevel set to : ");
  Serial.println(battery_level);
}

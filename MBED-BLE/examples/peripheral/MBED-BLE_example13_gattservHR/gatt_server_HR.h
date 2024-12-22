/*
 *
 *  ********************************************************
 *
 * A number of routines are coming from Mbed with the following
 * license information
 *
 * mbed Microcontroller Library
 * Copyright (c) 2018 ARM Limited
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
 * taken from
 *
 * https://github.com/ARMmbed/mbed-os-example-ble/tree/master/BLE_GattServer_AddService
 *
 * Adjusted for Sparkfun Artemis / version 1.0 / paulvha / January  2022
 */

#include "mbed_ble.h"
#include "heartrateSrv.h"

#ifndef BLE_GATTSRVHR_H
#define BLE_GATTSRVHR_H

void update_value();      // in sketch
uint16_t _heartrate_value;// make value public for update from sketch

class GattServHR : private mbed::NonCopyable<GattServHR>, public ble::Gap::EventHandler{
public:
    GattServHR(BLE &ble, events::EventQueue &event_queue) :
        _ble(ble),
        _event_queue(event_queue),
        _heartrate_uuid(GattService::UUID_HEART_RATE_SERVICE),
        _heartrate_service(ble, _heartrate_value, HeartRateService::LOCATION_FINGER),
        _adv_data_builder(_adv_buffer)
    {
    }

    void start()
    {
        _ble.init(this, &GattServHR::on_init_complete);

        _event_queue.dispatch_forever();
    }

    /** Set Name of device */
    void set_device_name(char * n)
    {
        strncpy(_device_name,n,20);
    }

private:
    /** Callback triggered when the ble initialization process has finished */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *params)
    {
        if (params->error != BLE_ERROR_NONE) {
            printf("Ble initialization failed.");
            return;
        }

        print_mac_address();

        /* this allows us to receive events like onConnectionComplete() */
        _ble.gap().setEventHandler(this);

        /* heart rate value updated every second */
        _event_queue.call_every(
            1000ms,
            [this] {
                update_sensor_value();
            }
        );

        start_advertising();
    }

    void start_advertising()
    {
        /* Create advertising parameters and payload */

        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(100))
        );

        _adv_data_builder.setFlags();
        _adv_data_builder.setAppearance(ble::adv_data_appearance_t::GENERIC_HEART_RATE_SENSOR);
        _adv_data_builder.setLocalServiceList({&_heartrate_uuid, 1});
        _adv_data_builder.setName(_device_name);

        /* Setup advertising */

        ble_error_t error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters
        );

        if (error) {
            printf("_ble.gap().setAdvertisingParameters() failed\r\n");
            return;
        }

        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            printf("_ble.gap().setAdvertisingPayload() failed\r\n");
            return;
        }

        /* Start advertising */

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            printf("_ble.gap().startAdvertising() failed\r\n");
            return;
        }

        printf("Heart rate sensor advertising, please connect\r\n");
    }

    void update_sensor_value(){

        if(! IsConnected) return;

        update_value(); // IN sketch.ino

        _heartrate_service.updateHeartRate(_heartrate_value);
    }

private:
    /* when we connect we stop advertising, restart advertising so others can connect */
    virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
    {
        if (event.getStatus() == ble_error_t::BLE_ERROR_NONE) {
          printf("Connected to: ");
          print_address(event.getPeerAddress());

        }
        IsConnected = true;
        printf("Client connected, you may now subscribe to updates\r\n");
    }

    /* when we connect we stop advertising, restart advertising so others can connect */
    virtual void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event)
    {
        printf("Client disconnected, restarting advertising\r\n");
        IsConnected = false;
        ble_error_t error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            printf("_ble.gap().startAdvertising() failed\r\n");
            return;
        }
    }

private:
    BLE &_ble;
    events::EventQueue &_event_queue;

    UUID _heartrate_uuid;
    HeartRateService _heartrate_service;

    uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
    ble::AdvertisingDataBuilder _adv_data_builder;

    char _device_name[20] = "GattServer";

    bool IsConnected = false;
};

#endif

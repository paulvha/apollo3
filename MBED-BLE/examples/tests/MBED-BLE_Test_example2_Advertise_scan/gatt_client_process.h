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
 */

/*****************************************************************************
 * this file contains overrides to ble_process.h and brings: 
 * Switching between scanning and advertising
 * Select a scanned device based on the name
 * Start connection to the selected device
 *********************************************************************************/
#ifndef GATT_CLIENT_PROCESS_H_
#define GATT_CLIENT_PROCESS_H_

#include "ble_process.h"

using namespace std::literals::chrono_literals;

/* Scanning happens repeatedly and is defined by:
 *  - The scan interval which is the time (in 0.625us) between each scan cycle.
 *  - The scan window which is the scanning time (in 0.625us) during a cycle.
 * If the scanning process is active, the local device sends scan requests
 * to discovered peer to get additional data.
 */
static const ble::ScanParameters scan_params(
    ble::phy_t::LE_1M,
    ble::scan_interval_t(0x80),
    ble::scan_window_t(0x50),
    true, /* active scanning */
    ble::own_address_type_t::PUBLIC  /* can not use random */
);

bool Inscan = true;          // indicates the state : scanning (true) or advertising (false)

/**
 * Simple GattClient wrapper.
 * It will alternate between advertising and scanning to obtain a connection to GattServer.
 */
class GattClientProcess : public BLEProcess
{
public:
    GattClientProcess(events::EventQueue &event_queue, BLE &ble_interface) :
        BLEProcess(event_queue, ble_interface)
    {
    }

    /** Name we advertise as */
    const char* get_device_name() override
    {
        static const char name[] = "GattClient";
        return name;
    }

    /** Name of device we want to connect to */
    const char* get_peer_device_name()
    {
        return peer_device_name;
    }

    /** Set Name of device we want to connect to */
    void set_peer_device_name(char * n)
    {
        strncpy(peer_device_name,n,20);
    }
 
private:
    /** Alternate between scanning and advertising */
    virtual void start_activity()
    {
        if (Inscan) {
            _event_queue.call([this]() { start_scanning(); });
        } else {
            _event_queue.call([this]() { start_advertising(); }); //ble_proces.h
        }
        Inscan = !Inscan;
        _is_connecting = false;
    }

    /** scan for Peer Device */
    void start_scanning()
    {
        _ble.gap().setScanParameters(scan_params);

        ble_error_t ret = _ble.gap().startScan(ble::scan_duration_t(ble::millisecond_t(4000)));
        if (ret == ble_error_t::BLE_ERROR_NONE) {
            printf("Started scanning for \"%s\"\r\n", get_peer_device_name());
        } else {
            printf("Starting scan failed\r\n");
        }
    }

    /** Restarts main activity */
    void onScanTimeout(const ble::ScanTimeoutEvent &event) override {
        start_activity();
    }

    /** Check advertising report for name and connect to any device with the name to look for */
    void onAdvertisingReport(const ble::AdvertisingReportEvent &event) override {
        /* don't bother with analysing scan result if we're already connecting */
        if (_is_connecting) {
            return;
        }

        if (!event.getType().connectable()) {
            /* we're only interested in connectable devices */
            return;
        }

        ble::AdvertisingDataParser adv_data(event.getPayload());

        /* parse the advertising payload, looking for a discoverable device */
        while (adv_data.hasNext()) {
            ble::AdvertisingDataParser::element_t field = adv_data.next();

            /* connect to a discoverable device */
            if (field.type == ble::adv_data_type_t::COMPLETE_LOCAL_NAME) {

                if (field.value.size() == strlen(get_peer_device_name()) &&
                    (memcmp(field.value.data(), get_peer_device_name(), field.value.size()) == 0)) {

                    printf("We found \"%s\", connecting...\r\n", get_peer_device_name());

                    ble_error_t error = _ble.gap().stopScan();

                    if (error) {
                        print_error(error, "Error caused by Gap::stopScan");
                        return;
                    }
                    
                    error = _ble.gap().connect(
                        event.getPeerAddressType(),
                        event.getPeerAddress(),
                        ble::ConnectionParameters()
                        /*.setScanParameters(
                            ble::phy_t::LE_1M,
                            ble::scan_interval_t(96), // max
                            ble::scan_window_t(48)    //min
                        )
                        .setConnectionParameters(
                            ble::phy_t::LE_1M,
                            ble::conn_interval_t(6),     // minConnectionInterval
                            ble::conn_interval_t(12),    // maxConnectionInterval
                            ble::slave_latency_t(0),
                            ble::supervision_timeout_t(200),
                            ble::conn_event_length_t(4),  // minEventLength
                            ble::conn_event_length_t(6)   // maxEventLength
                        )*/
                        .setOwnAddressType(ble::own_address_type_t::PUBLIC)  // use PUBLIC as it will fail otherwise with RANDOM
                    );

                    if (error) {
                        _ble.gap().startScan();
                        return;
                    }

                    /* we may have already scan events waiting
                     * to be processed so we need to remember
                     * that we are already connecting and ignore them */
                    _is_connecting = true;

                    return;
                }
            }
        }
    }
    
private:
    bool _is_connecting = false;
    char peer_device_name[20] = "GattServer";
    
};

#endif /* GATT_CLIENT_PROCESS_H_ */

/*
 *
 * ********************************************************
 *
 * A number of routines are coming from Mbed examples with the following
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
 * Parts taken from
 *
 * https://github.com/ARMmbed/mbed-os-example-ble/tree/master/BLE_GattServer_AddService
 *
 * Adjusted paulvha
 *
 * Version 1.0 / January 2022
 */

/*****************************************************************************
 * this file contains:
 * Setting the devicename to advertise
 * advertising the BME280 service
 * Call StoreDataReceived() in the sketch for data/command received from Central/Client
 *****************************************************************************/

#include "mbed_ble.h"
#include "Bme280Srv.h"

#ifndef BLE_GATTSRVHR_H
#define BLE_GATTSRVHR_H

int _BmeCmd;                // command to write to central/client
struct data_to_exchange p;      // store BME280 data
extern void StoreDataReceived(const uint8_t *data, uint8_t len); // in sketch

class GattServBME : private mbed::NonCopyable<GattServBME>, public ble::Gap::EventHandler{
public:
  GattServBME(BLE &ble, events::EventQueue &event_queue) :
    _ble(ble),
    _event_queue(event_queue),
    _Bme280Service(ble, _BmeCmd, &p),
    _BME_uuid(UUID(BME280Service)),
    _adv_data_builder(_adv_buffer)
  {
  }

  void start()
  {
    _ble.init(this, &GattServBME::on_init_complete);
    _event_queue.dispatch_forever();
  }

  /** Set Name of device */
  void set_device_name(char * n)
  {
    strncpy(_device_name,n,20);
  }

  /* is central/client connected
   * true : connected
   */
  bool IsConnected()
  {
    return _IsConnected;
  }

  /* send BME280 data or command to central */
  void SendToCentral(bool act = true)
  {
   if(! _IsConnected) return;

   if (act) _Bme280Service.updateBme280(&p);
   else _Bme280Service.updateBmecmd(_BmeCmd);
  }

private:
  /* Callback triggered when the ble initialization process has finished */
  void on_init_complete(BLE::InitializationCompleteCallbackContext *params)
  {
    if (params->error != BLE_ERROR_NONE) {
        printf("Ble initialization failed.");
        return;
    }

    print_mac_address();

    /* this allows us to receive events like onConnectionComplete() */
    _ble.gap().setEventHandler(this);

    _ble.gattServer().onDataWritten(this, &GattServBME::onDataWritten);

    start_advertising();
  }

  /* called when data is received from Central */
  virtual void onDataWritten(const GattWriteCallbackParams *params)
  {
    /*
    // debug only
    printf(" handle %d\r\n",params->handle ); //= alertLevelChar.getValueHandle())
    printf(" connHandle %d\r\n",params->connHandle );
    printf(" writeOp %d\r\n",params->writeOp );
    printf(" offset %d\r\n",params->offset );
    printf(" len %d\r\n",params->len );
    printf(" error_code %d\r\n",params->error_code );
    printf(" data %d\r\n",params->data );
    */

    // in sketch
    StoreDataReceived(params->data, params->len);
  }

  void start_advertising()
  {
    /* Create advertising parameters and payload */

    ble::AdvertisingParameters adv_parameters(
      ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
      ble::adv_interval_t(ble::millisecond_t(100))
    );

    _adv_data_builder.setFlags();
    _adv_data_builder.setName(_device_name);
    _adv_data_builder.setLocalServiceList({&_BME_uuid, 1});

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

    printf("BME280 sensor advertising, please connect\r\n");
  }

private:
  /* when we connect we stop advertising, restart advertising so others can connect */
  virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
  {
    if (event.getStatus() == ble_error_t::BLE_ERROR_NONE) {
      printf("Connected to: ");
      print_address(event.getPeerAddress());
     _IsConnected = true;
     printf("Client connected, you may now subscribe to updates\r\n");
    }
    else {
      printf("Error connecting\r\n");
    }
    
  }

  /* when we connect we stop advertising, restart advertising so others can connect */
  virtual void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event)
  {
    printf("Client disconnected, restarting advertising\r\n");
    _IsConnected = false;
    ble_error_t error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

    if (error) {
      printf("_ble.gap().startAdvertising() failed\r\n");
      return;
    }
  }

private:
  BLE &_ble;
  events::EventQueue &_event_queue;

  UUID _BME_uuid;
  Bme280Service _Bme280Service;

  uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
  ble::AdvertisingDataBuilder _adv_data_builder;

  char _device_name[20] = "GattServer";

  bool _IsConnected = false;
};

#endif

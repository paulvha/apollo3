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
 * Version 1.0 / February 2022
 */

/*****************************************************************************
 * this file contains:
 * Setting the devicename to advertise
 * advertising the AMDTP service
 * Call the transfer protocol routines to handle multi block transfer
 *****************************************************************************/

#ifndef BLE_GATT_SRV_AMDTP_H
#define BLE_GATT_SRV_AMDTP_H

// hardcoded call back to sketch
extern void StoreDataReceived(uint8_t *data, uint16_t len);

// wait max 15 seconds
static const std::chrono::milliseconds TimeOutSending = 15000ms;

class GattServAMDTP : private mbed::NonCopyable<GattServAMDTP>, public ble::Gap::EventHandler{
public:

  GattServAMDTP(BLE &ble, events::EventQueue &event_queue) :
    _ble(ble),
    _tp(ble),
    _event_queue(event_queue),
    _AMDTP_uuid(UUID(ATT_UUID_AMDTP_SERVICE)),
    _adv_data_builder(_adv_buffer)
  {
  }

  /**
   *  Start the service
   */
  void start()
  {
    _ble.init(this, &GattServAMDTP::on_init_complete);
    _tp.amdtps_init();

    // set call back for data received
    _tp.on_data_received(mbed::callback(this, &GattServAMDTP::Data_From_AMDTP));
    _event_queue.dispatch_forever();
  }

  /**
   *  Set Name of device
   */
  void set_device_name(char * n)
  {
    strncpy(_device_name,n,20);
  }

  /**
   * is central / client connected
   * true : connected
   */
  bool IsConnected()
  {
    return _IsConnected;
  }

  /**
   * set as soon as advertising is started
   * reset when connected
   */
  bool IsAdvertising()
  {
    return _IsAdvertising;
  }

 /**
  * Send data to central with AMDTP
  *
  * return:
  * -2  Not connected
  * -1  error during sending
  *  0  sending of package has been completed
  *  1  Pending to Send next chunk of data
  */

  /* send data to central */
  int SendToCentral(uint8_t *sdata, uint16_t slen)
  {
    if (! _IsConnected) return -2;

    // send to AMDTP to handle
    int ret = _tp.AmdtpSendData(sdata, slen);

    if (ret == -1 ) {

        printf("%s: Failed to sent data for Central\n",__FILE__);

        // reset all pointers of the amdtp service
        _tp.amdtps_init();
    }

    // sending in chunks, set timeout on receiving
    else if (ret == 1) {
        cancelhandle = _event_queue.call_in(TimeOutSending,[this]() { TimedOutSend(); });
    }

    return(ret);
  }

 /**
   * Check that all packages have been sent (in case multiple chunk are needed)
   *
   * Return
   * True : all have been send
   * False : still busy sending chunks of data
   *
   */
  bool IsSendingComplete()
  {

    if (_tp.AmdtpSendComplete()){

        // cancel timeout check
        if (cancelhandle > 0) {
            _event_queue.cancel(cancelhandle);
            cancelhandle = 0;
        }
        return true;
    }

    return false;
  }

 /**
   * call back from AMDTP when data is ready to be returned.
   *
   * @param[in] cb The callback object that will be called when data is ready
   */
 void Data_From_AMDTP(uint8_t *data, uint16_t len)
 {
   // hardcoded callback to sketch
   StoreDataReceived(data, len);
 }

private:

  /* Callback triggered when the ble initialization process has finished */
  void on_init_complete(BLE::InitializationCompleteCallbackContext *params)
  {
    if (params->error != BLE_ERROR_NONE) {
        printf("%s: Ble initialization failed.",__FILE__);
        return;
    }

    /* this allows us to receive events like onConnectionComplete() */
    _ble.gap().setEventHandler(this);

    _ble.gattServer().onDataWritten(this, &GattServAMDTP::onDataWritten);

    start_advertising();
  }

  /* called when data is received from Central / client */
  virtual void onDataWritten(const GattWriteCallbackParams *params)
  {

    // debug only
/*
    printf("%s :onDataWritten : handle %d\r\n",__FILE__,params->handle );
    printf(" connHandle %d\r\n",params->connHandle );
    printf(" writeOp %d\r\n",params->writeOp );
    printf(" offset %d\r\n",params->offset );
    printf(" len %d\r\n",params->len );
    printf(" error_code %d\r\n",params->error_code );
*/

    // send to AMDTP
    _tp.AmdtpReceivePkt(params->handle, params->len, (uint8_t *) params->data);
  }

 /**
   * timeout is set when sending in multipacket mode
   */
  void TimedOutSend()
  {
      if (_tp.AmdtpSendComplete()){
        if (cancelhandle > 0) {
            _event_queue.cancel(cancelhandle);
            cancelhandle = 0;
        }

      }
      else {
        printf("%s: Failed to get response data from Central\n",__FILE__);
        // reset all pointers of the amdtp service
        _tp.amdtps_init();
      }
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
    _adv_data_builder.setLocalServiceList({&_AMDTP_uuid, 1});

    /* Setup advertising */

    ble_error_t error = _ble.gap().setAdvertisingParameters(
      ble::LEGACY_ADVERTISING_HANDLE,
      adv_parameters
    );

    if (error) {
      printf("%s: setAdvertisingParameters() failed\r\n",__FILE__);
      return;
    }

    error = _ble.gap().setAdvertisingPayload(
      ble::LEGACY_ADVERTISING_HANDLE,
      _adv_data_builder.getAdvertisingData()
    );

    if (error) {
      printf("%s: setAdvertisingPayload() failed\r\n",__FILE__);
      return;
    }

    /* Start advertising */
    error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

    if (error) {
      printf("%s: startAdvertising() failed\r\n",__FILE__);
      return;
    }

    _IsAdvertising = true;
  }

  /* when we connect we stop advertising, restart advertising so others can connect */
  virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
  {
    if (event.getStatus() == ble_error_t::BLE_ERROR_NONE) {
     _IsConnected = true;
     _IsAdvertising = false;
    }
    else {
      printf("%s: Error connecting\r\n",__FILE__);
    }
  }

  /* when we connect we stop advertising, restart advertising so others can connect */
  virtual void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event)
  {
    printf("Client disconnected, restarting advertising\r\n");
    _IsConnected = false;

    // reset the amdtps
    _tp.amdtps_init();

    ble_error_t error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

    if (error) {
      printf("%s startAdvertising() failed\r\n",__FILE__);
      return;
    }
  }

private:
  BLE &_ble;
  events::EventQueue &_event_queue;
  AMDTPS _tp;
  UUID _AMDTP_uuid;

  uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
  ble::AdvertisingDataBuilder _adv_data_builder;

  char _device_name[20] = "GattServer";

  bool _IsConnected = false;
  bool _IsAdvertising = false;
  int cancelhandle = 0;
};

#endif //BLE_GATT_SRV_AMDTP_H

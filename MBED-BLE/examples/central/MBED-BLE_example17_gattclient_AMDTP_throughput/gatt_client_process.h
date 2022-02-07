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
  The AMDTP is based on the source code that is provided with AmbiqSuiteSDK. More information is available
  in the extras folder of the library.

  The sketch and connection to the server / peripheral is partly based on
  https://os.mbed.com/teams/mbed-os-examples/code/mbed-os-example-ble-GattClient/file/71d7cec222eb/main.cpp/
  
  But has many more features added

  Version 1.0 / February 2022 / paulvha
 */

/*****************************************************************************
 * this file contains: 
 * Initialize the MBED BLE stack
 * Set peripheral devicename to look for
 * Call a function after ble.init or connection or disconnect has completed
 * Select a scanned device based on the device name
 * Start connection to the selected device
 * Restart scan after disconnect
 *****************************************************************************/

#ifndef GATT_CLIENT_PROCESS_H_
#define GATT_CLIENT_PROCESS_H_

#include "amdtpc.h"

// get some debug messages from this file
//#define GattClientProcessDebug 1

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
    ble::own_address_type_t::PUBLIC  /* can not use random as the library has set a unique address*/
);

// set for 10 seconds scan
static const ble::scan_duration_t scan_duration(ble::millisecond_t(10000));

// the GattClientAMDTP constructor
GattClientAMDTP _CL_AMDTP;

/**
 * Simple GattClient wrapper.
 * It will scanning to obtain a connection to GattServer.
 */
class GattClientProcess : private mbed::NonCopyable<GattClientProcess>, public ble::Gap::EventHandler
{
public:
  GattClientProcess(events::EventQueue &event_queue, BLE &ble_interface) :
    _event_queue(event_queue),
    _ble(ble_interface),
    _gap(ble_interface.gap())
  {
  }
  
  ~GattClientProcess()
  {
    stop();
  }

 /**
   * Initialize the ble interface, configure it and start advertising.
   */
  void start()
  {
#ifdef GattClientProcessDebug
    printf("Client start.\r\n");
#endif

    if (_ble.hasInitialized()) {
      printf("Error: the ble instance has already been initialized. freeze\r\n");
      while(1);
    }

    /* handle gap events */
    _gap.setEventHandler(this);

    /* This will inform us off all events so we can schedule their handling
     * using our event queue */
    _ble.onEventsToProcess(
      makeFunctionPointer(this, &GattClientProcess::schedule_ble_events)
    );

    /* once it's done it will let us continue with our _CL_AMDTP */
    on_init(mbed::callback(&_CL_AMDTP, &GattClientAMDTP::start));
    on_connect(mbed::callback(&_CL_AMDTP, &GattClientAMDTP::start_discovery));
    on_disconnect(mbed::callback(&_CL_AMDTP, &GattClientAMDTP::stop));


    ble_error_t error = _ble.init(this, &GattClientProcess::on_init_complete);

    if (error) {
      print_error(error, "Error returned by _ble.init. Freeze");
      while(1);
    }

    // Process the event queue.
    _event_queue.dispatch_forever();
  }  
  
  /**
   * Close existing connections and stop the process.
   */
  void stop()
  {
    if (_ble.hasInitialized()) {
      _ble.shutdown();
      printf("Ble process stopped.");
    }
  }
  
  /** Get name of device we want to connect to */
  const char* get_peer_device_name()
  {
      return peer_device_name;
  }

  /** Set Name of device we want to connect to */
  void set_peer_device_name(char * n)
  {
      strncpy(peer_device_name,n,20);
  }
  
  /**
   * Subscription to the ble interface initialization event.
   *
   * @param[in] cb The callback object that will be called when the ble interface is initialized.
   */
  void on_init(mbed::Callback<void(BLE&, events::EventQueue&)> cb)
  {
    _post_init_cb = cb;
  }

  /**
   * Set callback for a succesful connection.
   *
   * @param[in] cb The callback object that will be called when we connect to a peer
   */
  void on_connect(mbed::Callback<void(BLE&, events::EventQueue&, const ble::ConnectionCompleteEvent &event)> cb)
  {
    _post_connect_cb = cb;
  }

  /**
   * Set callback for a succesful disconnection.
   *
   * @param[in] cb The callback object that will be called when we discconnect from a peer
   */
  void on_disconnect(mbed::Callback<void(BLE&, events::EventQueue&)> cb)
  {
    _post_disconnect_cb = cb;
  }
  
  /*
   * return true if connected
   */
  bool IsConnected()
  {
    return _is_connected;
  }

  /* 
   * send to server, keep single interface on user level 
   * 
   *  -1 error during sending
   *  0 sending completed
   *  1 sending in chunks of data
   */
  int SendToServer(uint8_t *sdata, uint16_t slen)
  {
    return (_CL_AMDTP.SendToServer(sdata,slen));
  }

 /*
  * check that sending (chunks) is complete
  * return True all has been send
  */
  bool IsSendingComplete()
  {
    return(_CL_AMDTP.IsSendingComplete());
  }
  
  /* 
   *  keep single interface on user level
   *  True if all AMDTP services have been found
   */
  
  bool ConnectionComplete()
  {
    return (_CL_AMDTP.ConnectionComplete());
  }
  
private:

  /**
   * Sets up adverting payload and start advertising.
   * This function is invoked when the ble interface is initialized.
   */
  void on_init_complete(BLE::InitializationCompleteCallbackContext *event)
  {
    if (event->error) {
      print_error(event->error, "Error during the initialisation. Freeze.");
      while(1);
    }
#ifdef GattClientProcessDebug
    printf("Ble instance initialized\r\n");
#endif
    /* All calls are serialised on the user thread through the event queue */
    start_activity();

    // set in sketch
    if (_post_init_cb) {
      _post_init_cb(_ble, _event_queue);
    }
  }

  /** start scanning */
  virtual void start_activity()
  {
    _event_queue.call([this]() { start_scanning(); });
    _is_connecting = false;
  }

  /** scan for Peer Device */
  void start_scanning()
  {
    _gap.setScanParameters(scan_params);

    ble_error_t ret = _gap.startScan(scan_duration);

    if (ret == BLE_ERROR_NONE) {
        printf("Started scanning for \"%s\"\r\n", peer_device_name);
    } else {
        print_error(ret, "Error returned by _gap.startScan");
    }
  }

  /** Restarts main activity */
  void onScanTimeout(const ble::ScanTimeoutEvent &event) override {
#ifdef GattClientProcessDebug
    printf("gatt_client, onscantimeout. Try again\r\n");
#endif
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
      if (field.type == ble::adv_data_type_t::COMPLETE_LOCAL_NAME || field.type == ble::adv_data_type_t::SHORTENED_LOCAL_NAME) {

        if (field.value.size() == strlen(peer_device_name) &&
          (memcmp(field.value.data(), peer_device_name, field.value.size()) == 0)) {

          printf("We found \"%s\", connecting...\r\n", peer_device_name);

          ble_error_t error = _gap.stopScan();

          if (error) {
            print_error(error, "Error caused by Gap::stopScan");
            return;
          }
          
          error = _gap.connect(
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
              print_error(error, "Error during _gap.connect");
              _gap.startScan();   // restart scanning
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

  /**
   * Start the gatt client process when a connection event is received.
   * This is called by Gap to notify the application we connected
   */
  void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override
  {
    if (event.getStatus() == BLE_ERROR_NONE) {
      printf("Connected to: ");
      print_address(event.getPeerAddress());
      _is_connected = true;
      
      // set in sketch
      if (_post_connect_cb) {
          _post_connect_cb(_ble, _event_queue, event);
      }
    } else {
      print_error(event.getStatus(), "Failed to connect");
      _is_connected = false;
      start_activity();
    }
  }

  /**
   * Stop the gatt client process when the device is disconnected then restart advertising.
   * This is called by Gap to notify the application we disconnected
   */
  void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override
  {
    printf("Disconnected.\r\n");
   
    // set in sketch
    if (_post_disconnect_cb) {
      _post_disconnect_cb(_ble, _event_queue);    // reset counters
    }
    _is_connected = false;
    start_activity();
  }

  /**
   * Schedule processing of events from the BLE middleware in the event queue.
   */
  void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *event)
  {
    _event_queue.call(mbed::callback(&event->ble, &BLE::processEvents));
  }
 
private:
  bool _is_connecting = false;
  bool _is_connected = false;
  char peer_device_name[20] = "GattServ";
  events::EventQueue &_event_queue;
  BLE &_ble;
  ble::Gap &_gap;
  
  mbed::Callback<void(BLE&, events::EventQueue&)> _post_init_cb;
  mbed::Callback<void(BLE&, events::EventQueue&)> _post_disconnect_cb;
  mbed::Callback<void(BLE&, events::EventQueue&, const ble::ConnectionCompleteEvent &event)> _post_connect_cb;
};

#endif /* GATT_CLIENT_PROCESS_H_ */

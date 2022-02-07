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
  The sketch and connection to the server / peripheral is partly based on
  https://os.mbed.com/teams/mbed-os-examples/code/mbed-os-example-ble-GattClient/file/71d7cec222eb/main.cpp/

  But has many more features added

  Version 1.0 / January 2022 / paulvha
 */
/*****************************************************************************
 * this file contains initializing AMDTP:
 * Discover services, characteristics
 * Optionally , add filter to select specific services
 * Optionally , add filter to select Command characteristic
 * Subscribe to Notify / indication charactaristics
 * Ability to write to command characteristic
 *****************************************************************************/

// uncomment to get process info from this file
//#define AMDTP_Gatt_Client_Debug 1

#include "GattClient.h"

#ifndef BLE_GATTCLIENT_H
#define BLE_GATTCLIENT_H

// maximum length of service UUID (16 bytes = 128 bits)
#define FILTERSIZE 16

// maximum filter service UUID's to store
#define MAXUUID 5

// offset to characteristic info
#define RX  0
#define TX  1
#define ACK 2

// hardcoded call back to Sketch
extern void StoreDataReceived(uint8_t *data, uint16_t len);

// wait max 15 seconds
static const std::chrono::milliseconds TimeOutSending = 15000ms;

/**
 * Handle discovery of the GATT server.
 *
 * First the GATT server is discovered in its entirety then each readable
 * characteristic is read and the client register to characteristic
 * notifications or indication when available. The client report server
 * indications and notification until the connection end.
 */
class GattClientAMDTP : private mbed::NonCopyable<GattClientAMDTP>, public GattClient::EventHandler
{
  // Internal typedef to this class type.
  // It is used as a shorthand to pass member function as callbacks.
  // keeping it readable...
  typedef GattClientAMDTP Self;
  typedef CharacteristicDescriptorDiscovery::DiscoveryCallbackParams_t DiscoveryCallbackParams_t;
  typedef CharacteristicDescriptorDiscovery::TerminationCallbackParams_t TerminationCallbackParams_t;
  typedef DiscoveredCharacteristic::Properties_t Properties_t;

  struct DiscoveredCharacteristicNode {
    DiscoveredCharacteristicNode(const DiscoveredCharacteristic &c) :
      value(c), next(nullptr) { }

    DiscoveredCharacteristic value;
    DiscoveredCharacteristicNode *next;
  };

public:

   /**
   * Construct an empty client process.
   *
   * The function start() shall be called to initiate the discovery process.
   */
  GattClientAMDTP():
  _tp()               // connection to the AMDTP client protocol
  { }

  ~GattClientAMDTP()
  {
    terminate();
  }

  /* called after init has been completed */
  void start(BLE &ble, events::EventQueue &event_queue)
  {
    _ble = &ble;
    _event_queue = &event_queue;
    _client = &_ble->gattClient();
    _gap = &_ble->gap();              // needed for terminate / disconnect

    // initialize the needed service and characteristics
    AddServiceFilter(String (ATT_UUID_AMDTP_SERVICE));

    AddChar(RX, String (ATT_UUID_AMDTP_RX));    // set characteristics to discover
    AddChar(TX, String (ATT_UUID_AMDTP_TX));
    AddChar(ACK, String (ATT_UUID_AMDTP_ACK));

    // set call back from AMDTP to sent formatted data on characteristics
    _tp.on_ACK_write(mbed::callback(this, &GattClientAMDTP::Write_ACK_Char));
    _tp.on_data_write(mbed::callback(this, &GattClientAMDTP::Write_data_Char));
    _tp.on_data_received(mbed::callback(this, &GattClientAMDTP::Data_From_AMDTP));
  }

  /**
   * called by user program for data to send with AMDTP
   * return :
   * -1 error during sending
   *  0 sending completed
   *  1 sending in chunks of data
   */
  int SendToServer(uint8_t *sdata, uint16_t slen)
  {
    // forward to AMDTP
    int ret = _tp.AmdtpSendData(sdata, slen);

    // sending in chunks, set timeout on receiving
    if (ret == 1) {
        cancelhandle = _event_queue->call_in(TimeOutSending,[this]() { TimedOutSend(); });
    }
    return ret;
  }

  /**
   * timeout is set when sending in multipacket mode
   */
  void TimedOutSend()
  {
      if (_tp.AmdtpSendComplete()){
        if (cancelhandle > 0) {
            _event_queue->cancel(cancelhandle);
            cancelhandle = 0;
        }

      }
      else {
        printf("%s: Failed to get response data from Central\n",__FILE__);
        // reset all pointers of the amdtp service
        _tp.amdtpc_init();
      }
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

        if (cancelhandle > 0) {
            _event_queue->cancel(cancelhandle);
            cancelhandle = 0;
        }
        return true;
    }

    return false;
  }

  /** parse a string to hex bytes in reverse order
   *
   *  @param input : string to parse
   *  @param output : array to store
   *
   *  return:
   *    length in bytes
   *    0 = error
   */
  int StringToHex(String input, uint8_t *output)
  {
    uint8_t i, cnt, h, t;
    uint8_t l = input.length();
    int ret;
    bool Nib_st = true;

    // determine length of input
    if (l == 17) ret = 6;      // address
    else if (l > 32) ret = 16; // UUID 128 bits
    else if (l > 4) ret = 4;   // UUID 32 bits
    else ret = 2;              // UUID 16 bits

    for(i = 0, cnt = 0; i < l; i++) {

      h = input[i];

      if (h == '-' || h == ':') continue;

      // ascii to hex
      if( h >= '0' && h <= '9') h = h - '0';
      else if( h >= 'a' && h <= 'f') h = h - 'a' + 0xa;
      else h = h - 'A' + 0xa;

      // store in single byte
      if (Nib_st) {
        t = (h & 0xf) << 4;
        Nib_st = !Nib_st;
      }
      else {
        t |= (h & 0xf);
        Nib_st = !Nib_st;

        output[cnt++] = t;
      }
    }

    if (! Nib_st ) {
      printf("%s: Error during translating \r\n",__FILE__);
      Serial.println(input);
      ret = 0;
    }

    return(ret);
  }

  /**
   * Add new services UUID to search list
   * MAXUUID unique UUID's can be added to the list
   * Could be called external
   */
  void AddServiceFilter(String sfilter)
  {
    uint8_t filter[16];
    int k, i = 0;

    // set UUID filter
    if (sfilter.length() == 0) return;

    k = StringToHex(sfilter, &filter[0]);

    if (k == 0)  return;

    // save UUID filter (reversed)
    for (int c = k-1 ; c > -1 ; c--) {
      // skip any dashes
      if (filter[c] != '-') _UUIDs[_NumUuid].u[i++] = filter[c];
    }

    // save length of UUID received (2, 4 or 16 bytes)
    _UUIDs[_NumUuid].ulen = k;

    // set for next entry else stay/overwrite last entry next time
    if ( _NumUuid < MAXUUID-1) _NumUuid++;

    _FilterSet = true;

    // only get data for service that match
    ServiceDetected = false;
  }

  /**
   * Add command characteristic UUID to find the handle
   * this can be used to send ACK / data from central/client to peripheral/server
   */
  void AddChar(uint8_t indx, String sfilter)
  {
    uint8_t filter[16];
    int k, i = 0;

    // set UUID filter
    if (sfilter.length() == 0) return;

    k = StringToHex(sfilter, &filter[0]);

    if (k == 0)  return;

    // save UUID filter (reversed)
    for (int c = k-1; c > -1 ; c--) {
      if (filter[c] != '-') _CharUUIDs[indx].u[i++] = filter[c];
    }
     // save length of UUID received (2, 4 or 16 bytes )
    _CharUUIDs[indx].ulen = k;
    _CharUUIDs[indx].handle = 0;
  }

 /**
  * write on a characteristic from central to peripheral.
  * return true if OK, else if error
  * to be called from AMDTP
  */
  bool WriteChar(uint8_t indx, uint8_t *data, uint16_t len)
  {
    if (_CharUUIDs[indx].handle != 0) {

      //printf("sending data to server ");
      //for (uint16_t i =0; i < len ; i++ ) printf("%x ", data[i]);
      //printf("\r\n");

      ble_error_t error = _client->write(
        GattClient::GATT_OP_WRITE_REQ,  //GATT_OP_WRITE_CMD,
        _connection_handle,
        _CharUUIDs[indx].handle,
        len,
        data
     );

     if (! error)  return true;
    }
    return false;
  }

  /**
   * Write Acknowledgement
   * while there is an acknowledgement characteristic some BLE stack
   * reply to all when a notify characteristic is updated. hence we
   * now use the RX (for server/peripheral) characteristic
   */
  void Write_ACK_Char(uint8_t *data, uint16_t len)
  {
    WriteChar(RX, data, len);
  }

  void Write_data_Char(uint8_t *data, uint16_t len)
  {
    WriteChar(RX, data, len) ;    // what is RX for server is TX for client
  }

  /**
   *  call back from AMDTP with received data packet
   * hardcode function in Sketch will be called
   */
  void Data_From_AMDTP(uint8_t *data, uint16_t len)
  {
    // hardcoded
    StoreDataReceived(data, len);
  }

  /**
   *  get a characteristic handle based on index (RX,TX or ACK
   */
  uint8_t GetCmdHandle(uint8_t indx)
  {
    return _CharUUIDs[indx].handle;
  }

  /**
   *  check whether connection and  discovery is complete
   * return
   * true is completed
   */
  bool ConnectionComplete()
  {
    return _DiscoverySuccess;
  }

  /**
   * Start the discovery process. (to be called when connected)
   *
   * @param[in] client The GattClient instance which will discover the distant GATT server.
   * @param[in] connection_handle Reference of the connection to the GATT
   * server which will be discovered.
   */
  void start_discovery(BLE &ble_interface, events::EventQueue &event_queue, const ble::ConnectionCompleteEvent &event)
  {
    _connection_handle = event.getConnectionHandle();
    _DiscoverySuccess = false;

    // setup the event handlers called during the process
    _client->onDataWritten().add(as_cb(&Self::when_descriptor_written));
    _client->onHVX().add(as_cb(&Self::when_characteristic_changed));

    // The discovery process will invoke when_service_discovered when a
    // service is discovered, when_characteristic_discovered when a
    // characteristic is discovered and when_service_discovery_ends once the
    // discovery process has ended.
    _client->onServiceDiscoveryTermination(as_cb(&Self::when_service_discovery_ends));

    ble_error_t error = _client->launchServiceDiscovery(
      _connection_handle,
      as_cb(&Self::when_service_discovered),
      as_cb(&Self::when_characteristic_discovered)
    );

    if (error) {
      print_error(error, "Error _client->launchServiceDiscovery.");
      return;
    }

    // register as a handler for GattClient events
    _client->setEventHandler(this);

    // this might not result in a new value but if it does we will be informed through
    // an call in the event handler we just registered
    // the desired-att-mtu is set to 23 and only if run a peripheral it might changed
    _client->negotiateAttMtu(_connection_handle);

#ifdef AMDTP_Gatt_Client_Debug
    printf("Client process started: initiate service discovery\r\n");
#endif
  }

  /**
   * Stop the discovery process and clean the instance.
   * make sure to terminate first else crash
   */
  void stop(BLE &ble, events::EventQueue &event_queue)
  {
    if (!_client) {
      return;
    }

    // unregister event handlers
    _client->onDataWritten().detach(as_cb(&Self::when_descriptor_written));
    _client->onHVX().detach(as_cb(&Self::when_characteristic_changed));
    _client->onServiceDiscoveryTermination(nullptr);

    // remove discovered characteristics
    clear_characteristics();

    // clean up the instance
    _connection_handle = 0;
    _characteristics = nullptr;
    _it = nullptr;
    _descriptor_handle = 0;
    _DiscoverySuccess = false;

#ifdef AMDTP_Gatt_Client_Debug
    printf("Client process stopped.\r\n");
#endif
  }

private:

  /**
   *  terminate connection
   *  from disconnect stop() is called to clean up
   *  then it will re-start scan again
   */
  void terminate()
  {
    printf("Terminating connnection\r\n");
    _gap->disconnect(_connection_handle, ble::local_disconnection_reason_t::USER_TERMINATION);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Service and characteristic discovery process.
  ///////////////////////////////////////////////////////////////////////////////

  /**
   * Handle services discovered.
   *
   * The GattClient invokes this function when a service has been discovered.
   *
   * @see GattClient::launchServiceDiscovery
   */
  void when_service_discovered(const DiscoveredService *discovered_service)
  {
    uint8_t i, j;

    // if service filter was set
    // Although you can add a UUID filter with the discovery call.. it is only ONE service
    // with this implementation you can add more services to discover

    if (_FilterSet){

      ServiceDetected = false;

      const UUID &s_uuid = discovered_service->getUUID();
      const uint8_t *uuid_value = s_uuid.getBaseUUID();
      bool match;

      // each registrated UUID
      for (j = 0; j < _NumUuid ; j++){

        match = true;

        // was length is not the same as for the filter
        if ( s_uuid.getLen() != _UUIDs[j].ulen ) continue;

        // check on UUID match
        for (i = 0; i < _UUIDs[j].ulen;  i++) {
         //printf("UUIID %x, value %x\n", _UUIDs[j].u[i], uuid_value[i]);
          if (_UUIDs[j].u[i] != uuid_value[i]) match = false;
        }

        // we have a match
        if (match) {
          ServiceDetected = true;
          j = _NumUuid;
        }
      }
    }

    if (ServiceDetected) {
      printf("AMDTP Service discovered\r\n");
#ifdef AMDTP_Gatt_Client_Debug
      printf("value = ")
      print_uuid(discovered_service->getUUID());
      printf("value = , start = %u, end = %u.\r\n",
          discovered_service->getStartHandle(),
          discovered_service->getEndHandle()
      );
#endif
    }
  }

  /**
   * Handle characteristics discovered.
   *
   * The GattClient invoke this function when a characteristic has been
   * discovered.
   *
   * @see GattClient::launchServiceDiscovery
   */
  void when_characteristic_discovered(const DiscoveredCharacteristic *discovered_characteristic)
  {
    // ignore if (optional) service(s) was not discovered before
    if (! ServiceDetected) return;

#ifdef AMDTP_Gatt_Client_Debug
    printf("Characteristic discovered: uuid = ");
    print_uuid(discovered_characteristic->getUUID());
    printf(", properties = ");
    print_properties(discovered_characteristic->getProperties());
    printf(", decl handle = %u, value handle = %u, last handle = %u.\r\n",
      discovered_characteristic->getDeclHandle(),
      discovered_characteristic->getValueHandle(),
      discovered_characteristic->getLastHandle()
    );
#endif

    // add the characteristic into the list of discovered characteristics
    bool success = add_characteristic(discovered_characteristic);
    if (!success) {
      printf("Error: memory allocation failure while adding the discovered characteristic.\r\n");
      _client->terminateServiceDiscovery();
      terminate();
      return;
    }

    // find the needed characteristics
    uint8_t indx;

    for (indx = RX; indx <= ACK ; indx++) {

      // Characteristic set and NOT found yet ?
      if (_CharUUIDs[indx].ulen > 0 && _CharUUIDs[indx].handle == 0){

        const UUID &s_uuid = discovered_characteristic->getUUID();
        const uint8_t *uuid_value = s_uuid.getBaseUUID();
        bool match = true;

        // length the same as for the characteristic
        if ( s_uuid.getLen() == _CharUUIDs[indx].ulen ) {

          // check on UUID match
          for (uint8_t i = 0; i < _CharUUIDs[indx].ulen;  i++) {
            // printf("UUIID %x, value %x\n", _UUIDs[j].u[i], uuid_value[i]);
            if (_CharUUIDs[indx].u[i] != uuid_value[i]) match = false;
          }

          // we have a match
          if (match) {
            _CharUUIDs[indx].handle = discovered_characteristic->getValueHandle();

#ifdef AMDTP_Gatt_Client_Debug
            printf("\tCharacteristic %d discovered: Handle  = %d\r\n",indx, _CharUUIDs[indx].handle);
#endif
          }
        }
      }
    }
  }

  /**
   * Handle termination of the service and characteristic discovery process.
   *
   * The GattClient invokes this function when the service and characteristic
   * discovery process ends.
   *
   * @see GattClient::onServiceDiscoveryTermination
   */
  void when_service_discovery_ends(ble::connection_handle_t connection_handle)
  {
    if (!_characteristics) {
      printf("No characteristics discovered, end of the process.\r\n");
      return;
    }

#ifdef AMDTP_Gatt_Client_Debug
    printf("All services and characteristics discovered, process them.\r\n");
#endif

    // reset iterator and start processing characteristics in order
    _it = nullptr;
    _event_queue->call(mbed::callback(this, &Self::process_next_characteristic));
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Processing of characteristics based on their properties.
  ////////////////////////////////////////////////////////////////////////////////

  /**
   * Process the characteristics discovered.
   *
   * - If the characteristic is readable then read its value and print it. Then
   * - If the characteristic can emit notification or indication then discover
   *   the characteristic CCCD and subscribe to the server initiated event.
   * - Otherwise skip the characteristic processing.
   */
  // #define READPROP 1 // enable reading during init (debug only)
  void process_next_characteristic(void)
  {
    if (!_it) _it = _characteristics;
    else  _it = _it->next;

    while (_it) {
      Properties_t properties = _it->value.getProperties();

#if READPROP    // enable reading during init
      if (properties.read()) {
        read_characteristic(_it->value);
        return;
      } else
#endif
      if(properties.notify() || properties.indicate()) {
        discover_descriptors(_it->value);
        return;
      } else {

#ifdef AMDTP_Gatt_Client_Debug
        printf(
          "Skip processing of characteristic %u\r\n",
          _it->value.getValueHandle()
        );
#endif
        _it = _it->next;
      }
    }

#ifdef AMDTP_Gatt_Client_Debug
    printf("All characteristics discovered have been processed.\r\n");
#endif

    // did we find all the characteristics
    if (_CharUUIDs[RX].handle && _CharUUIDs[TX].handle && _CharUUIDs[ACK].handle) {
      _tp.amdtpc_init();
      _DiscoverySuccess = true;
    }
  }

  /**
   * Initiate the read of the characteristic in input.
   *
   * The completion of the operation will happens in when_characteristic_read()
   */
  void read_characteristic(const DiscoveredCharacteristic &characteristic)
  {

#ifdef AMDTP_Gatt_Client_Debug
    printf("Initiating read at %u.\r\n", characteristic.getValueHandle());
#endif

    ble_error_t error = characteristic.read(0, as_cb(&Self::when_characteristic_read));

    if (error) {
      printf("Error: cannot initiate read at %u due to %u\r\n",
        characteristic.getValueHandle(), error);
      terminate();
    }
  }

  /**
   * Handle the reception of a read response.
   *
   * If the characteristic can emit notification or indication then start the
   * discovery of the the characteristic descriptors then subscribe to the
   * server initiated event by writing the CCCD discovered. Otherwise start
   * the processing of the next characteristic discovered in the server.
   */
  void when_characteristic_read(const GattReadCallbackParams *read_event)
  {

#ifdef AMDTP_Gatt_Client_Debug
    printf("\tCharacteristic value at %u equal to: ", read_event->handle);
    for (size_t i = 0; i <  read_event->len; ++i) {
      printf("0x%02X ", read_event->data[i]);
    }
    printf("\r\n");
#endif

    Properties_t properties = _it->value.getProperties();

    if(properties.notify() || properties.indicate()) {
      discover_descriptors(_it->value);
    } else {
      process_next_characteristic();
    }
  }

  /**
   * Initiate the discovery of the descriptors of the characteristic in input.
   *
   * When a descriptor is discovered, the function when_descriptor_discovered
   * is invoked.
   */
  void discover_descriptors(const DiscoveredCharacteristic &characteristic)
  {

#ifdef AMDTP_Gatt_Client_Debug
    printf("Initiating descriptor discovery of %u.\r\n", characteristic.getValueHandle());
#endif

    _descriptor_handle = 0;
    ble_error_t error = characteristic.discoverDescriptors(
      as_cb(&Self::when_descriptor_discovered),
      as_cb(&Self::when_descriptor_discovery_ends)
    );

    if (error) {
      printf(
        "Error: cannot initiate discovery of %04X due to %u.\r\n",
        characteristic.getValueHandle(), error
      );
      terminate();
    }
  }

  /**
   * Handle the discovery of the characteristic descriptors.
   *
   * If the descriptor found is a CCCD then stop the discovery. Once the
   * process has ended subscribe to server initiated events by writing the
   * value of the CCCD.
   */
  void when_descriptor_discovered(const DiscoveryCallbackParams_t* event)
  {

#ifdef AMDTP_Gatt_Client_Debug
    printf("\tDescriptor discovered at %u, UUID: ", event->descriptor.getAttributeHandle());
    print_uuid(event->descriptor.getUUID());
    printf(".\r\n");
#endif

    if (event->descriptor.getUUID() == BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG) {
      _descriptor_handle = event->descriptor.getAttributeHandle();
      _client->terminateCharacteristicDescriptorDiscovery(
        event->characteristic
      );
    }
  }

  /**
   * If a CCCD has been found subscribe to server initiated events by writing
   * its value.
   */
  void when_descriptor_discovery_ends(const TerminationCallbackParams_t *event)
  {
    // shall never happen but happen with android devices ...
    // process the next charateristic
    if (!_descriptor_handle) {
#ifdef AMDTP_Gatt_Client_Debug
      printf("Warning: characteristic with notify or indicate attribute without CCCD.\r\n");
#endif
      process_next_characteristic();
      return;
    }

    Properties_t properties = _it->value.getProperties();

    uint16_t cccd_value =
      (properties.notify() << 0) | (properties.indicate() << 1);

    ble_error_t error = _client->write(
      GattClient::GATT_OP_WRITE_REQ,
      _connection_handle,
      _descriptor_handle,
      sizeof(cccd_value),
      reinterpret_cast<uint8_t*>(&cccd_value)
    );

    if (error) {
      printf("Error: cannot initiate write of CCCD %u due to %u.\r\n",
          _descriptor_handle, error);
      terminate();
    }
#ifdef AMDTP_Gatt_Client_Debug
    printf("CCCD written value %x, %x, handle %d\r\n",cccd_value, _descriptor_handle);
#endif

  }

  /**
   * Called when the CCCD has been written.
   */
  void when_descriptor_written(const GattWriteCallbackParams* event)
  {
#ifdef AMDTP_Gatt_Client_Debug
    printf("Error code : 0x%x or %d\r\n", event->error_code, event->error_code);
#endif
    if (_descriptor_handle == event->handle) {

#ifdef AMDTP_Gatt_Client_Debug
      printf("CCCD at %u written.\r\n", _descriptor_handle);
#endif

      _descriptor_handle = 0;
      process_next_characteristic();
    }

    else if (_CharUUIDs[RX].handle == event->handle) {

#ifdef AMDTP_Gatt_Client_Debug
      printf("RX characteristic written at %u.\r\n", _descriptor_handle);
#endif

    }

    else if (_CharUUIDs[TX].handle == event->handle) {

#ifdef AMDTP_Gatt_Client_Debug
      printf("TX characteristic written at %u.\r\n", _descriptor_handle);
#endif

    }
    else if (_CharUUIDs[ACK].handle == event->handle) {

#ifdef AMDTP_Gatt_Client_Debug
      printf("ACK characteristic written at %u.\r\n", _descriptor_handle);
#endif

    }

    else {
      // should never happen
      printf("Error: received write response to unsolicited request.\r\n");
      terminate();
      return;
    }
  }

  /**
   * Print the updated value of the characteristic.
   *
   * This function is called when the server emits a notification or an
   * indication of a characteristic value the client has subscribed to.
   *
   * @see GattClient::onHVX()
   */
  void when_characteristic_changed(const GattHVXCallbackParams* event)
  {

#ifdef AMDTP_Gatt_Client_Debug
    printf("Change on attribute %u: new value = ", event->handle);
    for (size_t i = 0; i < event->len; ++i) {
        printf("0x%02X ", event->data[i]);
    }
    printf("\r\n");
#endif

    // send to AMDTP
    _tp.AmdtpReceivePkt(event->handle, event->len, (uint8_t *) event->data);
  }

  /**
   * Add a discovered characteristic into the list of discovered characteristics.
   */
  bool add_characteristic(const DiscoveredCharacteristic *characteristic)
  {
    DiscoveredCharacteristicNode* new_node =
      new(std::nothrow) DiscoveredCharacteristicNode(*characteristic);

    if (new_node == nullptr) {
      printf("Error while allocating a new characteristic.\r\n");
      return false;
    }

    if (_characteristics == nullptr) {
      _characteristics = new_node;
    } else {
      DiscoveredCharacteristicNode* c = _characteristics;
      while(c->next) {
        c = c->next;
      }
      c->next = new_node;
    }

    return true;
  }

  /**
   * Clear the list of discovered characteristics.
   */
  void clear_characteristics(void)
  {
    DiscoveredCharacteristicNode *c= _characteristics;

    while (c) {
      DiscoveredCharacteristicNode *n = c->next;
      delete c;
      c = n;
    }
  }

  /**
  * Implementation of GattClient::EventHandler::onAttMtuChange event
  * TODO :
  */
  virtual void onAttMtuChange(
    ble::connection_handle_t connectionHandle,
    uint16_t attMtuSize
    )
    {
      printf(
          "ATT_MTU changed on the connection %d to a new value of %d.\r\n",
          connectionHandle,
          attMtuSize
          /* maximum size of an attribute written in a single operation is one less */
      );
    }

  /**
   * Helper to construct an event handler from a member function of this
   * instance.
   */
  template<typename ContextType>
  FunctionPointerWithContext<ContextType> as_cb(void (Self::*member)(ContextType context))
  {
    return makeFunctionPointer(this, member);
  }

private:
  BLE *_ble = nullptr;
  events::EventQueue *_event_queue = nullptr;
  GattClient *_client = nullptr;
  Gap *_gap = nullptr;
  AMDTPC _tp; // = nullptr;

  ble::connection_handle_t _connection_handle = 0;
  DiscoveredCharacteristicNode *_characteristics = nullptr;
  DiscoveredCharacteristicNode *_it = nullptr;
  GattAttribute::Handle_t _descriptor_handle = 0;
  bool _DiscoverySuccess = false;    // true if discovery completed

  // filter variables
  struct UUIDr {
    uint8_t u[FILTERSIZE];            // uuid services filter
    int     ulen;                     // length of UUID service filter
  };

  bool _FilterSet = false;            // a service filter request was set
  uint8_t _NumUuid = 0;               // number of UUID filters
  struct UUIDr _UUIDs[MAXUUID];       // holds UUID services filters
  bool ServiceDetected = true;        // Only add characteristics for Service match detected

  struct CharUuid {
    uint8_t u[FILTERSIZE];            // Characteristic uuid
    int     ulen = 0 ;                // length of UUID
    uint8_t handle = 0;               // handle of characteristic
  };

  struct CharUuid _CharUUIDs[3];

  // store the user instructed function for call back
  mbed::Callback<void(uint8_t *data, uint16_t len)> _on_data_cb;

  int cancelhandle = 0;
};


#endif //BLE_GATTCLIENT_H

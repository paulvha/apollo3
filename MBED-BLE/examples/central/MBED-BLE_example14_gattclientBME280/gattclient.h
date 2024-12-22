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
 * Parts added by paulvha
 * Version 1.0 / January 2022
 */
/*****************************************************************************
 * this file contains: 
 * Discover services, characteristics
 * Optionally , add filter to select specific services
 * Optionally , add filter to select Command characteristic
 * Subscribe to Notify / indication charactaristics
 * Ability to write to command characteristic
 *****************************************************************************/

// uncomment to get process info
//#define GattClientInfo 1

#include "GattClient.h"
#ifndef BLE_GATTCLIENT_H
#define BLE_GATTCLIENT_H

// maximum length of service UUID (16 bytes = 128 bits)
#define FILTERSIZE 16

// maximum filter service UUID's to store
#define MAXUUID 5    

// defined in the sketch and called when Notify data has been received
extern void On_data_received(const GattHVXCallbackParams* event);

/**
 * Handle discovery of the GATT server.
 *
 * First the GATT server is discovered in its entirety then each readable
 * characteristic is read and the client register to characteristic
 * notifications or indication when available. The client report server
 * indications and notification until the connection end.
 */
class GattClientDemo : private mbed::NonCopyable<GattClientDemo>, public GattClient::EventHandler
{
  // Internal typedef to this class type.
  // It is used as a shorthand to pass member function as callbacks.
  // keeping it readable...
  typedef GattClientDemo Self;
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
  GattClientDemo() { }

  ~GattClientDemo()
  {
    terminate();
  }

  /* called after init has been completed */
  void start(BLE &ble, events::EventQueue &event_queue)
  {
    _ble = &ble;
    _event_queue = &event_queue;
    _client = &_ble->gattClient();
    _gap = &_ble->gap();              // needed for terminate/ disconnect
  }

  /* Optional  !
   * Add new services UUID to search list
   * MAXUUID unique UUID's can be added to the list
   */
  void AddServiceFilter(uint8_t *filter, uint8_t len)
  {
    uint8_t _FilterLen = len, i;

    if (_FilterLen > FILTERSIZE) _FilterLen = FILTERSIZE;
 
    // save UUID filter (reversed)
    for (int c = _FilterLen-1, i = 0 ; c > -1 ; c--, i++) _UUIDs[_NumUuid].u[i] = filter[c];
  
    // save length of UUID received ( 2, 4 or 16 bytes )
    _UUIDs[_NumUuid].ulen = _FilterLen;
    
    // set for next entry else stay/overwrite last entry next time
    if ( _NumUuid < MAXUUID-1) _NumUuid++;

    _FilterSet = true;

    // only get data for service that match
    ServiceDetected = false;
  }
  
  /* Add command characteristic UUID to find the handle  
   * this can be used to send command from central/client to peripheral/server
   * Feedback will be received with Notify
   */
  void AddCmdChar(uint8_t *CmdChar, uint8_t len)
  {
    uint8_t _CmdLen = len, i;

    if (_CmdLen > FILTERSIZE) _CmdLen = FILTERSIZE;
 
    // save UUID filter (reversed)
    for (int c = _CmdLen-1, i = 0 ; c > -1 ; c--, i++) _CmdCharUUID.u[i] = CmdChar[c];
     // save length of UUID received ( 2, 4 or 16 bytes )
    _CmdCharUUID.ulen = _CmdLen;
    _CmdCharUUID.handle = 0;
  }

 /* write to command from central to peripheral. 
  * return true if OK, else if error
  */
  bool WriteCmdChar( uint8_t *data, uint8_t len)
  {
    if (_CmdCharUUID.handle != 0) {
    
      ble_error_t error = _client->write(
        GattClient::GATT_OP_WRITE_CMD,
        _connection_handle,
        _CmdCharUUID.handle,
        len,
        data
     );
   
     if (! error)  return true;
    }
    return false;
   }
 
 /* get command characteristic handle */
 uint8_t GetCmdHandle()
 {
  return _CmdCharUUID.handle;
 }

 /* check whether connectionand  discovery is complete 
  * true is completed*/
 bool ConnectionComplete()
 {
  return _DiscoverySuccess;
 }
 
/**
 * Start the discovery process. (called when connected)
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
  
#ifdef GattClientInfo
  printf("Client process started: initiate service discovery\r\n");
#endif
}

/** 
 *  terminate connection
 *  from disconnect stop() is called to clean up
 *  then it will call scan again
 */
void terminate()
{
  printf("Terminating connnection\r\n");
  _gap->disconnect(_connection_handle, ble::local_disconnection_reason_t::USER_TERMINATION);
}

/**
 * Stop the discovery process and clean the instance.
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
  
#ifdef GattClientInfo
  printf("Client process stopped.\r\n");
#endif
}
  
private:
////////////////////////////////////////////////////////////////////////////////
// Service and characteristic discovery process.

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

  // service filter was set
  // although you can add a UUID filter during discovery.. it is only ONE service
  // with this implementation you can add more services
  
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
        // printf("UUIID %x, value %x\n", _UUIDs[j].u[i], uuid_value[i]);
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
    printf("Service discovered: value = ");
    print_uuid(discovered_service->getUUID());
    printf(", start = %u, end = %u.\r\n",
        discovered_service->getStartHandle(),
        discovered_service->getEndHandle()
    );
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
  if (! ServiceDetected) return;
  
#ifdef GattClientInfo    
  printf("\tCharacteristic discovered: uuid = ");
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

  // CmdCharacteristic set and NOT found yet ?
  if (_CmdCharUUID.ulen > 0 && _CmdCharUUID.handle == 0){
    
    const UUID &s_uuid = discovered_characteristic->getUUID();
    const uint8_t *uuid_value = s_uuid.getBaseUUID();
    bool match = true;

    // length the same as for the characteristic
    if ( s_uuid.getLen() == _CmdCharUUID.ulen ) {
       
      // check on UUID match
      for (uint8_t i = 0; i < _CmdCharUUID.ulen;  i++) {
        // printf("UUIID %x, value %x\n", _UUIDs[j].u[i], uuid_value[i]);
        if (_CmdCharUUID.u[i] != uuid_value[i]) match = false;
      }
      
      // we have a match
      if (match) {
        _CmdCharUUID.handle = discovered_characteristic->getValueHandle();
#ifdef GattClientInfo
        printf("\tCommand Characteristic discovered: Handle  = %d\r\n", _CmdCharUUID.handle);
#endif
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
  
#ifdef GattClientInfo
  printf("All services and characteristics discovered, process them.\r\n");
#endif

  // reset iterator and start processing characteristics in order
  _it = nullptr;
  _event_queue->call(mbed::callback(this, &Self::process_next_characteristic));
}

////////////////////////////////////////////////////////////////////////////////
// Processing of characteristics based on their properties.

/**
 * Process the characteristics discovered.
 *
 * - If the characteristic is readable then read its value and print it. Then
 * - If the characteristic can emit notification or indication then discover
 *   the characteristic CCCD and subscribe to the server initiated event.
 * - Otherwise skip the characteristic processing.
 */
void process_next_characteristic(void)
{
  if (!_it) {
    _it = _characteristics;
  } else {
    _it = _it->next;
  }

  while (_it) {
    Properties_t properties = _it->value.getProperties();
   
    if (properties.read()) {
      read_characteristic(_it->value);
      return;
    } else if(properties.notify() || properties.indicate()) {
      discover_descriptors(_it->value);
      return;
    } else {
      
#ifdef GattClientInfo
      printf(
        "Skip processing of characteristic %u\r\n",
        _it->value.getValueHandle()
      );
#endif
      _it = _it->next;
    }
  }
  
#ifdef GattClientInfo
  printf("All characteristics discovered have been processed.\r\n");
#endif

  if (_CmdCharUUID.handle) {
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
  
#ifdef GattClientInfo
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
  
#ifdef GattClientInfo
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
  
#ifdef GattClientInfo
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
  
#ifdef GattClientInfo
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
#ifdef GattClientInfo
    printf("\tWarning: characteristic with notify or indicate attribute without CCCD.\r\n");
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
}

/**
 * Called when the CCCD has been written.
 */
void when_descriptor_written(const GattWriteCallbackParams* event)
{
  if (_descriptor_handle == event->handle) {
    
#ifdef GattClientInfo
    printf("CCCD at %u written.\r\n", _descriptor_handle);
#endif

    _descriptor_handle = 0;
    process_next_characteristic();    
  }
  
  else if (_CmdCharUUID.handle == event->handle) {

#ifdef GattClientInfo
    printf("Command written at %u.\r\n", _descriptor_handle);
#endif

  }
  else {
    // should never happen
    printf("\tError: received write response to unsolicited request.\r\n");
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
  
#ifdef GattClientInfo
  printf("Change on attribute %u: new value = ", event->handle);
  for (size_t i = 0; i < event->len; ++i) {
      printf("0x%02X ", event->data[i]);
  }
  printf("\r\n");
#endif

  // call routine to handle data in the sketch
  On_data_received(event);
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

  struct cmdcharuuid {
    uint8_t u[FILTERSIZE];            // CmdChar uuid
    int     ulen = 0 ;                // length of UUID 
    uint8_t handle = 0;               // handle of CmdChar
  };
    
   struct cmdcharuuid _CmdCharUUID;
};

#endif //BLE_GATTCLIENT_H

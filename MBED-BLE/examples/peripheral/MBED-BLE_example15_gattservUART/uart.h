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
 * Other parts have been developed by paulvha
 *
 * Version 1.0 / January 2022
 */

#include "mbed_ble.h"

#ifndef BLE_UART_H
#define BLE_UART_H

extern void On_data_received(const GattWriteCallbackParams* event);
const static char DEVICE_NAME[] = "BLE_Uart";

/* from Nordic Uart Service */
const uint8_t NUS_BASE_UUID[UUID::LENGTH_OF_LONG_UUID] = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x00, 0x00, 0x40, 0x6E};
const uint8_t BLE_UUID_NUS_SERVICE[UUID::LENGTH_OF_LONG_UUID] = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E};
const uint8_t BLE_UUID_NUS_TX_CHARACTERISTIC[UUID::LENGTH_OF_LONG_UUID] = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E};
const uint8_t BLE_UUID_NUS_RX_CHARACTERISTIC[UUID::LENGTH_OF_LONG_UUID] = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E};

// send a sort of keep alive every 20 seconds
static const std::chrono::milliseconds RATE_SND_MSG = 20000ms;

class BleUartDemo : ble::Gap::EventHandler {

public:
  /** Maximum length of data (in bytes) that the UART service module can transmit to the peer. */
  static const unsigned BLE_UART_SERVICE_MAX_DATA_LEN = (BLE_GATT_MTU_SIZE_DEFAULT - 3);

public:
  BleUartDemo(BLE &ble, events::EventQueue &event_queue) :
      _ble(ble),
      _event_queue(event_queue),
      _connected(false),
      _uart_uuid(UUID(NUS_BASE_UUID, UUID::LSB)),
      _adv_data_builder(_adv_buffer),
      receiveBuffer(),
      sendBuffer(),
      sendBufferIndex(0),
      numBytesReceived(0),
      receiveBufferIndex(0),
      txCharacteristic(/* UUID */ UUID(BLE_UUID_NUS_TX_CHARACTERISTIC, UUID::LSB),
                       /* Initial value */ receiveBuffer,
                       /* Value size */ 1,
                       /* Value capacity */ BLE_UART_SERVICE_MAX_DATA_LEN,
                       /* Properties */ GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
                       /* Descriptors */ NULL,
                       /* Num descriptors */ 0,
                       /* variable len */ true),
      rxCharacteristic(/* UUID */ UUID(BLE_UUID_NUS_RX_CHARACTERISTIC, UUID::LSB),
                       /* Initial value */ sendBuffer,
                       /* Value size */ 1,
                       /* Value capacity */ BLE_UART_SERVICE_MAX_DATA_LEN,
                       /* Properties */ GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE,
                       /* Descriptors */ NULL,
                       /* Num descriptors */ 0,
                       /* variable len */ true) 
  {
      GattCharacteristic *charTable[] = {&txCharacteristic, &rxCharacteristic};
      GattService uartService(UUID(BLE_UUID_NUS_SERVICE, UUID::LSB), charTable, sizeof(charTable) / sizeof(GattCharacteristic *));
      _ble.gattServer().addService(uartService);
  }

  ~BleUartDemo()
  {
    if (_ble.hasInitialized()) {
        _ble.shutdown();
    }
  }
  /* entry point to start the NOrdic Uart Service */
  void start() {
    
    _ble.gap().setEventHandler(this);
    
    _ble.init(this, &BleUartDemo::on_init_complete);

    // update regular on the line
   _cancel_handle = _event_queue.call_every(RATE_SND_MSG, this, &BleUartDemo::SendMessage);

    _event_queue.dispatch_forever();
  }

  uint16_t getTXCharacteristicHandle() {
      return txCharacteristic.getValueAttribute().getHandle();
  }

  // set a message to send
  bool SetMessage(uint8_t *inp, uint8_t len)
  {
    _event_queue.cancel(_cancel_handle);
    
    for (sendBufferIndex = 0; sendBufferIndex < len  && sendBufferIndex < BLE_UART_SERVICE_MAX_DATA_LEN ; sendBufferIndex++) {
        sendBuffer[sendBufferIndex] = inp[sendBufferIndex];
    }
    
    // set the message set
    SendMessage();

    // restart keep-alive updates
    _cancel_handle = _event_queue.call_every(RATE_SND_MSG, this, &BleUartDemo::SendMessage);
    return true;
  }

  // send a message on notify
  void SendMessage() 
  {

   uint8_t Resp[] = "You sent: ";    // reply received data if no keybaord input
   uint8_t data[] = "hello_";        // dummy payload
   static uint8_t cnt  = '0';        // serial number on dummy payload 
   
   // if No user keyboard input received
   if (sendBufferIndex == 0) {

    // if nothing received from remote
    if (numBytesReceived == 0) {
    
      // now send something to show we are alive
      for (uint8_t i = 0; i < sizeof(data); i++) sendBuffer[sendBufferIndex++] = data[i];
      sendBufferIndex--;                    // delete terminator
      sendBuffer[sendBufferIndex++] = cnt++;
      sendBuffer[sendBufferIndex] = 0x0;    // terminate
  
      if (cnt > '9') cnt = '0';
    }
      
    else { // echo the message from remote
        
      for (uint8_t i = 0; i < sizeof(Resp); i++) sendBuffer[sendBufferIndex++] = Resp[i];
      _uart->write(getTXCharacteristicHandle(), sendBuffer, sendBufferIndex, false);
      
      sendBufferIndex = 0;
      for (uint8_t i = 0; i < numBytesReceived; i++) sendBuffer[sendBufferIndex++] = receiveBuffer[i];
      
      // reset
      numBytesReceived = 0;
     }
    }
    
    _uart->write(getTXCharacteristicHandle(), sendBuffer, sendBufferIndex, false);
    
    // reset
    sendBufferIndex = 0;
  }

private:

  /** Callback triggered when the ble initialization process has finished */
  void on_init_complete(BLE::InitializationCompleteCallbackContext *params) {
      if (params->error != BLE_ERROR_NONE) {
          printf("Ble initialization failed.");
          return;
      }

      print_mac_address();

      _uart = &_ble.gattServer();
      _uart->onDataSent(this, &BleUartDemo::when_data_sent);
      _uart->onDataWritten(this, &BleUartDemo::when_data_written);

      start_advertising();
  }

  void start_advertising() {
    /* Create advertising parameters and payload */

    ble::AdvertisingParameters adv_parameters(
      ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
      ble::adv_interval_t(ble::millisecond_t(1000))
    );

    _adv_data_builder.setFlags();
    _adv_data_builder.setLocalServiceList(mbed::make_Span(&_uart_uuid, 1));
    _adv_data_builder.setName(DEVICE_NAME);

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

    printf("start advertising\r\n");
  }

private:
  /* Event handler */

  void onDisconnectionComplete(const ble::DisconnectionCompleteEvent&) {
    printf("Client disconnected, restarting advertising\r\n");
    _connected = false;
    
    ble_error_t error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

    if (error) {
      printf("_ble.gap().startAdvertising() failed\r\n");
      return;
    }
  }

  virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event) {
    if (event.getStatus() == BLE_ERROR_NONE) {
       printf("Connected to: ");
       print_address(event.getPeerAddress());
        _connected = true;
        printf("Client connected, you may now subscribe to updates\r\n");
    }
  }

  /**
   * Handler called when a notification or an indication has been sent.
   */
  void when_data_sent(unsigned count)
  {
    printf("sent %u updates\r\n", count);
    sendBufferIndex = 0;
  }

  /**
   * Handler called after an attribute has been written.
   */
  void when_data_written(const GattWriteCallbackParams *e)
  {
   numBytesReceived = 0;
    
    for (size_t i = 0; i < e->len; ++i) {
      
      // store received data in local buffer
      receiveBuffer[numBytesReceived++] = e->data[i];
      
      printf("%c", e->data[i]);
    }

    printf("\r\n");

    // call routine to handle data in the sketch
    On_data_received(e);

  }

private:
  BLE &_ble;
  events::EventQueue &_event_queue;

  int _cancel_handle = 0;
 
  bool _connected;

  UUID _uart_uuid;

  uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
  ble::AdvertisingDataBuilder _adv_data_builder;

  /* UART service */
  uint8_t  receiveBuffer[BLE_UART_SERVICE_MAX_DATA_LEN]; /**< The local buffer into which we receive
                                                          *   inbound data before forwarding it to the
                                                          *   application. */

  uint8_t  sendBuffer[BLE_UART_SERVICE_MAX_DATA_LEN];    /**< The local buffer into which outbound data is
                                                           *   accumulated before being pushed to the
                                                           *   rxCharacteristic. */
  uint8_t  sendBufferIndex;
  uint8_t  numBytesReceived;
  uint8_t  receiveBufferIndex;

  GattServer* _uart;

  GattCharacteristic  txCharacteristic; /**< From the point of view of the external client, this is the characteristic
                                         *   they'd write into in order to communicate with this application. */
  GattCharacteristic  rxCharacteristic; /**< From the point of view of the external client, this is the characteristic
                                         *   they'd read from in order to receive the bytes transmitted by this
                                         *   application. */
};

#endif

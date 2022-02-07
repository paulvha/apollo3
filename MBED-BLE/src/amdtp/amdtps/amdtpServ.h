/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
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
 * @brief AmbiqMicro Data Transfer Protocol service definition
 *
 * paulvha / January 2022
 */

#ifndef MBED_BLE_SVC_AMDTPS_H
#define MBED_BLE_SVC_AMDTPS_H

#include "amdtps_protocol.h"

////////////////////////////////////////////////////////////////////////////
// BLE settings
////////////////////////////////////////////////////////////////////////////

/*! Base UUID:  00002760-08C2-11E1-9073-0E8AC72EXXXX * /
#define ATT_UUID_AMDTP_SERVICE  "00002760-08C2-11E1-9073-0E8AC72E1011"
#define ATT_UUID_AMDTP_RX  "00002760-08C2-11E1-9073-0E8AC72E0011"
#define ATT_UUID_AMDTP_TX  "00002760-08C2-11E1-9073-0E8AC72E0012"
#define ATT_UUID_AMDTP_ACK "00002760-08C2-11E1-9073-0E8AC72E0013"
*/

#if BLE_FEATURE_GATT_SERVER

/**
 * AMDTP Service.
 *
 * @par purpose
 *
 * Implement an AMDTP (transfer protocol) service which will allow larger data packets
 * to be broken down in smaller packages and exchange between client and server.
 *
 * There are 3 characteristics :
 * TX for notifications from the server to the client
 * ACK for notifications from the server to the client
 * RX for ACK, control from the client to server
 *
 * @par usage
 *
 * When this class is instantiated, it adds a AMDTP service in the GattServer.
 * The service contains the a characteristics with the initial value zero value.
 *
 * Application code can invoke update() on the different characteristics when a new data
 *
 * @attention The AMDTP profile limits the number of instantiations of the
 * service to one.
 *
 *
 *  Version 1.0 / January 2022 / paulvha
 */

class AmdtpService {

public:
    /**
     * Construct the AMDTP service.
     *
     * The construction process adds a GATT AMDTP service in  @p _ble GattServer
     *
     * @param[in] _ble BLE device that hosts the AMDTP service.
     *
     */

    uint8_t data[1];    // dummy init load

    AmdtpService(BLE &ble) :
        _ble(ble),
        _TX(data,0),
        amdtp_TX(                // define characteristic
            UUID(ATT_UUID_AMDTP_TX),  // UUID
            _TX.getPointer(),         // pointer to data
            _TX.getNumValueBytes(),   // get number of current bytes
            S_TX::MAX_VALUE_BYTES,
            GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE
        ),
        _ACK(data,0),
        amdtp_ACK(                // define characteristic
            UUID(ATT_UUID_AMDTP_ACK),  // UUID
            _ACK.getPointer(),         // pointer to data
            _ACK.getNumValueBytes(),   // get number of current bytes
            S_ACK::MAX_VALUE_BYTES,
            GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY
        ),
        _RX(data,0),
        amdtp_RX(                // define characteristic
            UUID(ATT_UUID_AMDTP_RX),  // UUID
            _RX.getPointer(),         // pointer to data
            _RX.getNumValueBytes(),   // get number of current bytes
            S_RX::MAX_VALUE_BYTES,
            GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE
        )
    {
        setupService();
    }

    /**
     * Update the characteristic values
     *
     * The server sends a notification of the new value to clients that have
     * subscribed to updates of the AMDTP characteristic; clients
     * reading the measurement characteristic after the update obtain
     * the updated value.
     *
     * @param[data] pointer to updated data.
     * param[len] length of data.
     *
     * @attention This function must be called in the execution context of the
     * BLE stack.
     */
    void RX_update(uint8_t *data, uint8_t len) {
        _RX.update(data, len);
        _ble.gattServer().write(
            amdtp_RX.getValueHandle(),
            _RX.getPointer(),
            _RX.getNumValueBytes()
        );
       // printf("amdtpServ.h update RX: len %d handle : %d, numbytes %d\r\n", len, amdtp_RX.getValueHandle(), _RX.getNumValueBytes());
    }

    void TX_update(uint8_t *data, uint8_t len) {
        _TX.update(data, len);
        _ble.gattServer().write(
            amdtp_TX.getValueHandle(),
            _TX.getPointer(),
            _TX.getNumValueBytes()
        );
       // printf("amdtpServ.h update TX: handle %d, len %d handle : %d, numbytes %d\r\n", amdtp_TX.getValueHandle(), len, amdtp_TX.getValueHandle(), _TX.getNumValueBytes());
    }

    void ACK_update(uint8_t *data, uint8_t len) {
        _ACK.update(data, len);
        _ble.gattServer().write(
            amdtp_ACK.getValueHandle(),
            _ACK.getPointer(),
            _ACK.getNumValueBytes()
        );
       // printf("amdtpServ.h update ACK: len %d handle : %d, numbytes %d\r\n", len, amdtp_ACK.getValueHandle(), _ACK.getNumValueBytes());
    }

protected:
    /**
     * Construct and add to the GattServer AMDTP service.
     */
    void setupService(void) {
        GattCharacteristic *charTable[] = {
            &amdtp_RX,
            &amdtp_ACK,
            &amdtp_TX
        };

        GattService AMDTP_Service(
            UUID(ATT_UUID_AMDTP_SERVICE),
            charTable,
            sizeof(charTable) / sizeof(GattCharacteristic*)
        );

        _ble.gattServer().addService(AMDTP_Service);
    }

protected:

    /*
     * TX characteristic value.
     */
    struct S_TX {

      uint8_t NumBytes;

      /* enable structure size*/
      static const unsigned MAX_VALUE_BYTES = ATT_MAX_MTU;

      S_TX (uint8_t *data, uint8_t len) : valueBytes()
      {
        update(data, len);
      }

      // update the TX values
      void update(uint8_t *data, uint8_t len)
      {
        for (NumBytes = 0; NumBytes < len && NumBytes < ATT_MAX_MTU; NumBytes++)
          valueBytes[NumBytes] = data[NumBytes];
      }

      uint8_t *getPointer(void)
      {
        return valueBytes;
      }

      const uint8_t *getPointer(void) const
      {
        return valueBytes;
      }

      unsigned getNumValueBytes(void) const
      {
        return NumBytes;
      }

  private:
      uint8_t valueBytes[MAX_VALUE_BYTES];
  };

    /*
     * RX characteristic value.
     */
    struct S_RX {

      uint8_t NumBytes;

      /* enable structure size*/
      static const unsigned MAX_VALUE_BYTES = ATT_MAX_MTU;

      S_RX (uint8_t *data, uint8_t len) : valueBytes()
      {
        update(data, len);
      }

      // update the RX values
      void update(uint8_t *data, uint8_t len)
      {
        for (NumBytes = 0; NumBytes < len && NumBytes < ATT_MAX_MTU; NumBytes++)
          valueBytes[NumBytes] = data[NumBytes];
      }

      uint8_t *getPointer(void)
      {
        return valueBytes;
      }

      const uint8_t *getPointer(void) const
      {
        return valueBytes;
      }

      unsigned getNumValueBytes(void) const
      {
        return NumBytes;
      }

  private:
      uint8_t valueBytes[MAX_VALUE_BYTES];
  };

    /*
     * TX characteristic value.
     */
    struct S_ACK {

      uint8_t NumBytes;

      /* enable structure size*/
      static const unsigned MAX_VALUE_BYTES = AMDTP_ACK_SIZE;

      S_ACK (uint8_t *data, uint8_t len) : valueBytes()
      {
        update(data, len);
      }

      // update the values
      void update(uint8_t *data, uint8_t len)
      {
        for (NumBytes = 0; NumBytes < len && NumBytes < AMDTP_ACK_SIZE; NumBytes++)
          valueBytes[NumBytes] = data[NumBytes];
      }

      uint8_t *getPointer(void)
      {
        return valueBytes;
      }

      const uint8_t *getPointer(void) const
      {
        return valueBytes;
      }

      unsigned getNumValueBytes(void) const
      {
        return NumBytes;
      }

  private:
      uint8_t valueBytes[MAX_VALUE_BYTES];
  };

protected:
  BLE &_ble;
  S_RX _RX;
  S_TX _TX;
  S_ACK _ACK;

  GattCharacteristic amdtp_RX;
  GattCharacteristic amdtp_TX;
  GattCharacteristic amdtp_ACK;
};

#endif // BLE_FEATURE_GATT_SERVER


#endif /* MBED_BLE_SVC_AMDTPS_H*/

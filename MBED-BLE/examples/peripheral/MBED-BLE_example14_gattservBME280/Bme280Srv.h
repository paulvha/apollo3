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
 * Based om the Mbed-BLE Heartrate service.
 *
 * paulvha / January 2022
 */

#ifndef MBED_BLE_BME280_SERVICE_H__
#define MBED_BLE_BME280_SERVICE_H__

#include "ble/BLE.h"

/***********************************************************************/
// Define here the structure for the data to exchange.
// This structure must be defined the same on the central and peripheral
// max 20 bytes ( MTU size 23 - 3 overhead)
// used unions to stay within the 20 bytes.
/***********************************************************************/
struct data_to_exchange {
  // BME280 data
  float humidity;
  float pressure;
  float altitude;
  float temperature;
  union {
    uint8_t meter;       // if true altitude is in meter, else in feet
    uint8_t CmdStat;
  };
  union {
    uint8_t celsius;     // if true temperature is celsius, else fahrenheit
    uint8_t MagicNumber;
  };
};

#define MAGIC_CMD 0xCF      // indicate that notify data is command response

////////////////////////////////////////////////////////////////////////////
// BLE settings
////////////////////////////////////////////////////////////////////////////
//  BME280 service
char BME280Service[] = "19B10010-E8F2-537E-4F6C-D104768A1214";

// characteristic to allow remote device to read and write
char BME280_rw[] = "19B10011-E8F2-537E-4F6C-D104768A1214";

// characteristic to send notifications
char BME280_Not[]= "19B10012-E8F2-537E-4F6C-D104768A1214";

#if BLE_FEATURE_GATT_SERVER

/**
 * BME280 Service.
 *
 * @par purpose
 *
 * BME280 sensor can be used to measure the environment conditions of the location and temperature
 *
 * Clients can instruct the peripheral based on a menu. The service delivers
 * these updates to the subscribed client in a notification packet.
 *
 * The subscription mechanism is useful to save power; it avoids unecessary data
 * traffic between the client and the server, which may be induced by polling the
 * value of the BME280 characteristic.
 *
 * Optionally a peripheral can write the client a command on a different characteristic.
 *
 * @par usage
 *
 * When this class is instantiated, it adds a BME280 service in the GattServer.
 * The service contains the a characteristic with the initial value measured
 * by the sensor. It also adds a characteristic that can be used to exchange
 * commands between peripheral and central
 *
 * Application code can invoke updateBme() when a new BME280 measurements
 * are acquired; this function updates the value of the BME280 measurement
 * characteristic and notifies the new value to subscribed clients.
 *
 * @attention The BME280 profile limits the number of instantiations of the
 * service to one.
 *
 * Based on the heart-rate service from MBED-BLE
 */
class Bme280Service {

public:
    /**
     * Construct and initialize a BME280 service.
     *
     * The construction process adds a GATT BME280 service in  @p _ble GattServer
     *
     * @param[in] _ble BLE device that hosts the BME280 service.
     * @param[in] p a pointer to data_to_exchange structure
     * @param[in] cmd set for write/read to remote a command.
     *
     * ADDED BLE_GATT_CHAR_PROPERTIES_READ FOR BLEAK IMPLEMENTATION - paulvha
     */
    Bme280Service(BLE &_ble, uint8_t cmd, struct data_to_exchange *p) :
        ble(_ble),
        CmdvalueBytes(cmd),   // init command characteristic value
        bmeRW(                // define characteristic
            UUID(BME280_rw),  // UUID
            CmdvalueBytes.getPointer(),       // pointer to data (cmd byte)
            CmdvalueBytes.getNumValueBytes(), // get number of current bytes
            Bme280CmdBytes::MAX_VALUE_BYTES,
            GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ
        ),
        valueBytes(p),
        bmeNotity(
            UUID(BME280_Not),
            valueBytes.getPointer(),
            valueBytes.getNumValueBytes(),
            Bme280ValueBytes::MAX_VALUE_BYTES,
            GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY
        )
    {
        setupService();
    }

    /**
     * Update the BME280 that the service exposes.
     *
     * The server sends a notification of the new value to clients that have
     * subscribed to updates of the BME280 characteristic; clients
     * reading the measurement characteristic after the update obtain
     * the updated value.
     *
     * @param[in] pointer to updated structure.
     *
     * @attention This function must be called in the execution context of the
     * BLE stack.
     */
    void updateBme280(struct data_to_exchange *p) {
        valueBytes.updatebme(p);
        ble.gattServer().write(
            bmeNotity.getValueHandle(),
            valueBytes.getPointer(),
            valueBytes.getNumValueBytes()
        );
    }

    /**
     * send a command to the central. Optional as command does not need to be echoed over
     * this characteristic. instead Notify is used with a special magicnumber
     *
     * @param[in] cmd to to send.
     *
     * @attention This function must be called in the execution context of the
     * BLE stack.
     *
     * Sending this update works...but receiving on the MBED stack did not. MBED / CORDIO
     * only expects a response or Notify/indicate. Hence use Notify with special MagicNumber
     */
    /*
    void updateBmecmd(uint8_t cmd) {
        CmdvalueBytes.updateCmd(cmd);
        ble.gattServer().write(
            bmeRW.getValueHandle(),
            CmdvalueBytes.getPointer(),
            CmdvalueBytes.getNumValueBytes()
        );
    }
    */
   void updateBmecmd(uint8_t cmd) {
        valueBytes.updateCmd(cmd);
        ble.gattServer().write(
            bmeNotity.getValueHandle(),
            valueBytes.getPointer(),
            valueBytes.getNumValueBytes()
        );
    }

protected:
    /**
     * Construct and add to the GattServer the heart rate service.
     */
    void setupService(void) {
        GattCharacteristic *charTable[] = {
            &bmeRW,
            &bmeNotity
        };
        GattService bmeService(
            UUID(BME280Service),
            charTable,
            sizeof(charTable) / sizeof(GattCharacteristic*)
        );

        ble.gattServer().addService(bmeService);
    }

protected:
    /*
     * BME280 cmd value.
     * The central will send a byte to the peripheral
     * The other way around is not used. Feedback is over Notify
     */

    struct Bme280CmdBytes{
        /* 1 byte for cmd. */
        static const unsigned MAX_VALUE_BYTES = 1;

        Bme280CmdBytes(uint8_t cmd) : valueBytes()
        {
          updateCmd(cmd);
        }

        void updateCmd(uint8_t cmd)
        {
          valueBytes[0] = cmd;
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
          return sizeof(uint8_t);
        }

    private:
        uint8_t valueBytes[MAX_VALUE_BYTES];
    };

    /*
     * BME280 measurement value OR command feedback.
     * Will send an updated structure to the central
     */
    struct Bme280ValueBytes {

      /* enable structure size*/
      static const unsigned MAX_VALUE_BYTES = sizeof(struct data_to_exchange);

      Bme280ValueBytes(struct data_to_exchange *p) : valueBytes()
      {
        updatebme(p);
      }

      // update the BME280 values
      void updatebme(struct data_to_exchange *p)
      {
        struct data_to_exchange *n = (struct data_to_exchange *) getPointer();
        n->humidity = p->humidity;
        n->pressure = p->pressure;
        n->altitude = p->altitude;
        n->temperature = p->temperature;
        n->meter = p->meter;
        n->celsius = p->celsius;
      }

      // provide command feedback
      void updateCmd(uint8_t cmd)
      {
        struct data_to_exchange *n = (struct data_to_exchange *) getPointer();
        n->MagicNumber = MAGIC_CMD;
        n->CmdStat = cmd;
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
        return sizeof(struct data_to_exchange);
      }

  private:
      uint8_t valueBytes[MAX_VALUE_BYTES];
  };

protected:
  BLE &ble;
  Bme280ValueBytes valueBytes;
  Bme280CmdBytes CmdvalueBytes;
  GattCharacteristic bmeRW;
  GattCharacteristic bmeNotity;
};

#endif // BLE_FEATURE_GATT_SERVER

#endif /* #ifndef MBED_BLE_BME280_SERVICE_H__*/

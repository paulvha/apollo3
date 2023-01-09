/*
 * BLEUUID.h
 *
 *  Created on: Jun 21, 2017
 *      Author: kolban
 *      
 * Source ESP32 library
 * Adjusted for ArduinoBLE / January 2023 / paulvha
 */

#ifndef COMPONENTS_CPP_UTILS_BLEUUID_H_
#define COMPONENTS_CPP_UTILS_BLEUUID_H_

#include "Arduino.h"

/// UUID type
typedef struct {
#define ESP_UUID_LEN_16     2
#define ESP_UUID_LEN_32     4
#define ESP_UUID_LEN_128    16
    uint16_t len;              /*!< UUID length, 16bit, 32bit or 128bit */
    union {
        uint16_t    uuid16;
        uint32_t    uuid32;
        uint8_t     uuid128[ESP_UUID_LEN_128];
    } uuid;                 /*!< UUID */
} __attribute__((packed)) bt_uuid_t;

/**
 * @brief A model of a %BLE UUID.
 */
class BLEUUID {
public:
  // constructors
  BLEUUID(String uuid);
  BLEUUID(uint16_t uuid);
  BLEUUID(uint32_t uuid);
  BLEUUID(bt_uuid_t uuid);
  BLEUUID(uint8_t* pData, size_t size, bool msbFirst);
  BLEUUID();

  // functions
  uint8_t    bitSize();           // Get the number of bits in this uuid.
  bool       equals(BLEUUID uuid);
  bt_uuid_t* getNative();         // get point to m_uuid
  BLEUUID    to128();             // get the pointer to "this"
  String     toString();
  static BLEUUID fromString(String uuid);  // Create a BLEUUID from a string

private:
  bt_uuid_t  m_uuid;               // The underlying UUID structure that this class wraps.
  bool       m_valueSet = false;   // Is there a value set for this instance.
}; // BLEUUID

#endif /* COMPONENTS_CPP_UTILS_BLEUUID_H_ */

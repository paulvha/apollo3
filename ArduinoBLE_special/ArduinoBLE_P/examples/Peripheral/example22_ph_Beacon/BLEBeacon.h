/*
 * BLEBeacon2.h
 *
 *  Created on: Jan 4, 2018
 *      Author: kolban
 */

#ifndef COMPONENTS_CPP_UTILS_BLEBEACON_H_
#define COMPONENTS_CPP_UTILS_BLEBEACON_H_

#include "Arduino.h"
#include "BLEUUID.h"
/**
 * @brief Representation of a beacon.
 * See:
 * * https://en.wikipedia.org/wiki/IBeacon
 *
 * Source ESP32 library
 * Adjusted for ArduinoBLE / January 2023 / paulvha
 */

typedef struct Mbd {
      uint16_t manufacturerId;
      uint8_t  subType;
      uint8_t  subTypeLength;
      uint8_t  proximityUUID[16];
      uint16_t major;
      uint16_t minor;
      int8_t   signalPower;
}__attribute__((packed));

  
typedef union convMbd { 
      Mbd m_beaconData;
      uint8_t raw[sizeof(struct Mbd)];
};
 
class BLEBeacon {

private:
 
public:
  BLEBeacon();

  String      getData();
  int         getDataU(uint8_t *data, uint8_t datalen);
  uint16_t    getMajor();
  uint16_t    getMinor();
  uint16_t    getManufacturerId();
  BLEUUID     getProximityUUID();
  int8_t      getSignalPower();
  void        setData(String data);
  void        setMajor(uint16_t major);
  void        setMinor(uint16_t minor);
  void        setManufacturerId(uint16_t manufacturerId);
  void        setProximityUUID(BLEUUID uuid);
  void        setSignalPower(int8_t signalPower);
}; // BLEBeacon

#endif /* COMPONENTS_CPP_UTILS_BLEBEACON_H_ */

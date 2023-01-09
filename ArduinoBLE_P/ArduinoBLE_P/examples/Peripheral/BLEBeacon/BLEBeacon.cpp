/*
 * BLEBeacon.cpp
 *
 *  Created on: Jan 4, 2018
 *      Author: kolban
 *      
 * Source ESP32 library
 * Adjusted for ArduinoBLE / January 2023 / paulvha
 */

#include "BLEBeacon.h"

convMbd PP;

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00)>>8) + (((x)&0xFF)<<8))

BLEBeacon::BLEBeacon() {
  PP.m_beaconData.manufacturerId = 0x4c00; // Apple
  PP.m_beaconData.subType        = 0x02;   //(Apple's iBeacon type of Custom Manufacturer Data)
  PP.m_beaconData.subTypeLength  = 0x15;   // 21 = 16 (UUID) +2 (major), +2 (minor) + 1 (signalpower)
  PP.m_beaconData.major          = 0;
  PP.m_beaconData.minor          = 0;
  PP.m_beaconData.signalPower    = 0;      // 8 bit Signed value, ranges from -128 to 127, use Two's 
                                           // Compliment to "convert" if necessary, Units: Measured 
                                           // Transmission Power in dBm @ 1 meters from beacon. 
                                           // Set by user, not dynamic, can be used in conjunction 
                                           // with the received RSSI at a receiver to calculate 
                                           // rough distance to beacon.

  memset(PP.m_beaconData.proximityUUID, 0, sizeof(PP.m_beaconData.proximityUUID));
} // BLEBeacon

String BLEBeacon::getData() {
    
  String str;
  
  for (int j =0; j< sizeof(struct Mbd); j++) {
    str += (char)PP.raw[j];
  }  

  return (str);
} // getData

/**
 * Copy raw data as uint8_t
 * @param data : buffer to store
 * @param datalen : max size in buffer
 * 
 * return:
 * number of bytes copied
 */
int BLEBeacon::getDataU(uint8_t *data, uint8_t datalen) {
    
  int j;
  
  for (j = 0; j< sizeof(struct Mbd) & j < datalen; j++) {
    data[j] = PP.raw[j];
  }  

  return (j);
} // getDataU

uint16_t BLEBeacon::getMajor() {
  return PP.m_beaconData.major;
}

uint16_t BLEBeacon::getManufacturerId() {
  return PP.m_beaconData.manufacturerId;
}

uint16_t BLEBeacon::getMinor() {
  return PP.m_beaconData.minor;
}

BLEUUID BLEBeacon::getProximityUUID() {
    return BLEUUID(PP.m_beaconData.proximityUUID, 16, false);
}

int8_t BLEBeacon::getSignalPower() {
  return PP.m_beaconData.signalPower;
}

/**
 * Set the raw data for the beacon record.
 */
void BLEBeacon::setData(String data) {
  if (data.length() != sizeof(PP.m_beaconData)) {
    Serial.println("ERROR: Invalid length to setdata."); 
    return;
  }

  for(byte i = 0; i < sizeof(PP.m_beaconData);i++)
    PP.raw[i] = data.c_str()[i];

} // setData

void BLEBeacon::setMajor(uint16_t major) {
  PP.m_beaconData.major = ENDIAN_CHANGE_U16(major);
} // setMajor

void BLEBeacon::setManufacturerId(uint16_t manufacturerId) {
  PP.m_beaconData.manufacturerId = ENDIAN_CHANGE_U16(manufacturerId);
} // setManufacturerId

void BLEBeacon::setMinor(uint16_t minor) {
  PP.m_beaconData.minor = ENDIAN_CHANGE_U16(minor);
} // setMinior

void BLEBeacon::setProximityUUID(BLEUUID uuid) {
  uuid = uuid.to128();
  memcpy(PP.m_beaconData.proximityUUID, uuid.getNative()->uuid.uuid128, 16);
} // setProximityUUID

void BLEBeacon::setSignalPower(int8_t signalPower) {
  PP.m_beaconData.signalPower = signalPower;
} // setSignalPower

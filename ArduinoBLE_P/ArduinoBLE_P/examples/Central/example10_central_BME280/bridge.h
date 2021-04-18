/*
 * This file contains the routines that bridge between the sketch and Bluetooth
 *
 * Version 1.0 / February 2021 / paulvha
 * - initial version
 */

#ifndef _BLE_BRIDGE_H_
#define _BLE_BRIDGE_H_

#include <ArduinoBLE.h>
#include <Arduino.h>

//*****************************************************************************
//
// Settings 
//
//*****************************************************************************

/* BLE_DEBUG will show the routines and debug information
 * BLE_SHOW_DATA will show the data being received and sent
 *
 * uncomment / remove '//' to enable
 */
//#define BLE_Debug 1
//#define BLE_SHOW_DATA 1

/* ENABLE DATA with DEBUG */
#if defined BLE_Debug
#define BLE_SHOW_DATA 1
#endif

// define the serial port
#define SERIAL_PORT Serial

// size of receive buffer
#define MAXRECEIVEBUF   100

// UUID's
#define BME280_SERVICE  "19B10010-E8F2-537E-4F6C-D104768A1214"
#define BME280_RX  "19B10011-E8F2-537E-4F6C-D104768A1214"
#define BME280_TX  "19B10012-E8F2-537E-4F6C-D104768A1214"

#define AMD_IDLE      0
#define AMD_SCANNED   1
#define AMD_CONNECTED 2

//*****************************************************************************
//
// Forward declarations.
//
//*****************************************************************************
void RxChar_Received(BLEDevice central, BLECharacteristic characteristic);
bool get_handles();
bool PerformScan();
void perform_disconnect();

// sketch connect
void Start_BLE();
bool SendData(uint8_t *value, uint16_t vlen);
void HandlePeripheralResp(const uint8_t * buf, uint16_t len);
bool CheckConnect();

#endif // _BLE_BRIDGE_H_

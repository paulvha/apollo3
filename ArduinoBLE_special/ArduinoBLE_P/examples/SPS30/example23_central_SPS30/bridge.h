/*
 * This file contains the routines that bridge between the sketch and Bluetooth
 *
 * Version 1.0 / February 2021 / paulvha
 * - initial version
 */

#ifndef _BLE_BRIDGE_H_
#define _BLE_BRIDGE_H_

// CHANGE ArduinoBLE_P TO ArduinoBLE WHEN USING VERSION 2.X OF SPARKFUN LIBRARY
#include <ArduinoBLE_P.h>
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
#define DEV_SERVICE "19B10030-E8F2-537E-4F6C-D104768A1214"
#define DEV_RX  "19B10031-E8F2-537E-4F6C-D104768A1214"
#define DEV_TX  "19B10032-E8F2-537E-4F6C-D104768A1214"


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

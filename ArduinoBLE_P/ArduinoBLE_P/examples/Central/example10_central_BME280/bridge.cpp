/*
 * This file contains the routines that bridge between the sketch and Bluetooth
 *
 * Version 1.0 / February 2021 / paulvha
 * - initial version
 */

#include "bridge.h"

// keep track of connection status
uint8_t AMD_stat = AMD_IDLE;

BLECharacteristic RxChar;
BLECharacteristic TxChar;
BLEDevice peripheral;

//****************************************
//
// Receive handle function
//
//****************************************

void RxChar_Received(BLEDevice central, BLECharacteristic characteristic) {

  // in sketch
  HandlePeripheralResp(characteristic.value(),characteristic.valueSize());
}

//****************************************
//
// Transmit handle function over the TX characteristic
//
//****************************************
bool SendData(uint8_t *value, uint16_t vlen) {

  TxChar.writeValue(value, (int) vlen);

  return(true);
}

//****************************************
//
// Bluetooth functions
//
//****************************************

//
// perform poll for any BLE events and check for connected
//
bool CheckConnect()
{
  // poll the central for events
  BLE.poll();

  if ( !peripheral.connected()){
    AMD_stat = AMD_IDLE;
    return(false);
  }

  return(true);
}

//
// start Bluetooth and look for peripheral information
//
void Start_BLE()
{
#ifdef BLE_Debug
  BLE.debug(SERIAL_PORT);         // enable display HCI commands
#endif

  SERIAL_PORT.println(F("\rStarting Bluetooth"));

  AMD_stat = AMD_IDLE;

  if (!BLE.begin()){
    SERIAL_PORT.println(F("\rStarting BLE failed!"));
    while(1);
  }

  // find Peripheral
  if (!PerformScan()){
    SERIAL_PORT.println(F("\rCould not find Peripheral"));
    while(1);
  }

  // connect and find handles
  if (!get_handles()){
    SERIAL_PORT.println(F("\rCould not connect or find all characteristics"));
    while(1);
  }
}

//
// perform a disconnect
//
void perform_disconnect()
{
  if (AMD_stat == AMD_CONNECTED) BLE.disconnect();
  while (CheckConnect());
}

//
// scan for peripheral
//
bool PerformScan()
{
  if (AMD_stat != AMD_IDLE) return false;

  SERIAL_PORT.println(F("Scan for peripheral service\r"));

  unsigned long st = millis();

  // start scanning for AMDTP peripheral
  BLE.scanForUuid(BME280_SERVICE);

  while(1) {

    // check if a peripheral has been discovered
    peripheral = BLE.available();

    if (peripheral) {

      // discovered a peripheral
      SERIAL_PORT.println(F("Discovered an peripheral server\r"));
      SERIAL_PORT.println(F("-------------------------------\r"));

      // print address
      SERIAL_PORT.print(F("Address: "));
      SERIAL_PORT.print(peripheral.address());
      SERIAL_PORT.print("\r\n");

#ifdef BLE_Debug
      // print the local name, if present
      if (peripheral.hasLocalName()) {
        SERIAL_PORT.print(F("Local Name: "));
        SERIAL_PORT.print(peripheral.localName());
        SERIAL_PORT.print("\r\n");
      }

      // print the advertised service UUIDs, if present
      if (peripheral.hasAdvertisedServiceUuid()) {
        SERIAL_PORT.print(F("Service UUIDs: "));
        for (int i = 0; i < peripheral.advertisedServiceUuidCount(); i++) {
          SERIAL_PORT.print(peripheral.advertisedServiceUuid(i));
          SERIAL_PORT.print(" ");
        }
        SERIAL_PORT.print("\r\n");
      }

      // print the RSSI
      SERIAL_PORT.print(F("RSSI: "));
      SERIAL_PORT.println(peripheral.rssi());
      SERIAL_PORT.print("\r\n");
#endif

      AMD_stat = AMD_SCANNED;
      BLE.stopScan();
      return true;
    }

    // Try for max 10 seconds scan
    if (millis() - st > 10000) {
      BLE.stopScan();
      return false;
    }
  }
}

// connect and obtain handles
bool get_handles()
{
  if (AMD_stat != AMD_SCANNED) return false;

  // connect to the peripheral
  SERIAL_PORT.print(F("Connecting ..."));

  if (peripheral.connect()) {
    SERIAL_PORT.println(F("Connected\r"));
  }
  else {
    SERIAL_PORT.println(F("Failed to connect!\r"));
    return false;
  }

  AMD_stat = AMD_CONNECTED;

  // discover peripheral attributes
  SERIAL_PORT.print(F("Discovering attributes ..."));
  if (peripheral.discoverAttributes()) {
    SERIAL_PORT.println(F("Attributes discovered\r"));
  }
  else {
    SERIAL_PORT.println(F("Attribute discovery failed!\r"));
    perform_disconnect();
    return false;
  }

  // retrieve the characteristics
  RxChar = peripheral.characteristic(BME280_TX);  // TX peripheral is RX for HOST
  TxChar = peripheral.characteristic(BME280_RX);

  if (!RxChar || !TxChar ) {
    SERIAL_PORT.println(F("Peripheral does not have all characteristics!\r"));
    perform_disconnect();
    return false;
  }
  else if (!TxChar.canWrite()) {
    SERIAL_PORT.println(F("\rPeripheral does not have a writable characteristic!\r"));
    perform_disconnect();
    return false;
  }

  if (!RxChar.subscribe()) {
    SERIAL_PORT.println(F("RxCharacteristic notify subscription failed!\r"));
    peripheral.disconnect();
    return false;
  }

  // assign event handlers for characteristic
  RxChar.setEventHandler(BLEUpdated, RxChar_Received);
  return true;
}

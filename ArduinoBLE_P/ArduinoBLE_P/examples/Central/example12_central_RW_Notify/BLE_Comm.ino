/*
 * This file contains the routines Bluetooth Routines
 *
 * Version 1.0 / November  2022 / paulvha
 * - initial version
 */

# include "defines.h"

///////////////////////////////////////////////////////
// BLE defines
///////////////////////////////////////////////////////
const char BLE_PERIPHERAL_NAME[] = "Example12 BLE";

#define SERVICE "9e400001-b5a3-f393-e0a9-e12e24dcca9e"

//  characteristic read, notify and write
#define CHARACTERISTIC_R_UUID "9e400002-b5a3-f393-e0a9-e12e24dcca9e" // receive feedback
#define CHARACTERISTIC_W_UUID "9e400003-b5a3-f393-e0a9-e12e24dcca9e" // Send data characteristic
#define CHARACTERISTIC_N_UUID "9e400004-b5a3-f393-e0a9-e12e24dcca9e" // Notify characteristic

// handles
BLECharacteristic RX_Char;
BLECharacteristic TX_Char;
BLECharacteristic N_Char;
BLEDevice peripheral;

// how many Ms to wait for peripheral
#define ScanTimeOut 10000

////////////////
// machine state
////////////////
#define AMD_IDLE      0
#define AMD_SCANNED   1
#define AMD_CONNECTED 2

// keep track of connection status
uint8_t AMD_stat = AMD_IDLE;

/**
 * Receive handle function to read data
 * 
 * @param value : pointer to buffer to store
 * @param vlen  : max length to store
 * 
 * @ return     : length of bytes read
 * 
 */
int RXData(uint8_t *value, int vlen) {
  
  int lenc = RX_Char.readValue(value, vlen);

  return(lenc);
}


/**
 * send handle function
 * 
 * @param value : pointer to buffer to send
 * @param vlen  : max length to send
 * 
 * @ return     : length of bytes read
 * 
 */
bool TXData(uint8_t *value, int vlen) {

  TX_Char.writeValue(value,  vlen);
  return(true);
}

/**
 * Receive notifications call back
 */
void N_Char_Received(BLEDevice central, BLECharacteristic characteristic) {

  // in sketch
  HandlePeripheralNotify(characteristic.value(),characteristic.valueSize());
}
 


///////////////////////////////////////////////////////
// BLE functions
///////////////////////////////////////////////////////

/**
 * perform poll for any BLE events and check for connected
 */
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

/**
 * start Bluetooth and look for peripheral information
 */
void Start_BLE()
{
#ifdef BLE_Debug
  BLE.debug(SERIAL_PORT);         // enable display HCI commands
#endif

  SERIAL_PORT.println(F("\rStarting Bluetooth"));

  AMD_stat = AMD_IDLE;

  if (!BLE.begin()){
    SERIAL_PORT.println(F("\rStarting BLE failed! freeze."));
    while(1);
  }

  // find Peripheral by service name
  if (!PerformScan()){
    SERIAL_PORT.println(F("\rCould not find Peripheral. freeze."));
    while(1);
  }

  // connect and find handles
  if (!get_handles()){
    SERIAL_PORT.println(F("\rCould not connect or find all characteristics. freeze."));
    while(1);
  }

  // set peripheral disconnect handler
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
}

/**
 * force a disconnect
 */
void perform_disconnect()
{
  if (AMD_stat == AMD_CONNECTED) BLE.disconnect();
  while (CheckConnect());
}

/**
 * scan for peripheral by service UUID
 */
bool PerformScan()
{
  if (AMD_stat != AMD_IDLE) return false;

  SERIAL_PORT.println(F("Scan for peripheral service\r"));

  // prevent deadlock
  unsigned long st = millis();

  // start scanning for peripheral
  BLE.scanForUuid(SERVICE);

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

    // Try for max ScanTimeOut Ms scan
    if (millis() - st > ScanTimeOut) {
      BLE.stopScan();
      return false;
    }
  }
}

/**
 * connect and obtain handles
 */
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
  SERIAL_PORT.print(F("Discovering characteristics ..."));
  
  // retrieve the characteristics handles
  RX_Char = peripheral.characteristic(CHARACTERISTIC_W_UUID); // read data from peripheral
  TX_Char = peripheral.characteristic(CHARACTERISTIC_R_UUID); // send commands to peripheral
  N_Char = peripheral.characteristic(CHARACTERISTIC_N_UUID);  // Receive commands from peripheral

  if (!RX_Char || !TX_Char || !N_Char ) {
    SERIAL_PORT.println(F("Peripheral does not have all characteristics!\r"));
    perform_disconnect();
    return false;
  }
  else if (!TX_Char.canWrite() ) {
    SERIAL_PORT.println(F("\rPeripheral does not have a writable characteristic!\r"));
    perform_disconnect();
    return false;
  }

  if (!RX_Char.canRead() ) {
    SERIAL_PORT.println(F("\rPeripheral does not have a readable characteristic!\r"));
    peripheral.disconnect();
    return false;
  }

  if (!N_Char.subscribe()) {
    SERIAL_PORT.println(F("Set notify subscription failed!\r"));
    peripheral.disconnect();
    return false;
  }

  SERIAL_PORT.println(F("Discovered\r"));

  // assign event handlers for notify
  N_Char.setEventHandler(BLEUpdated, N_Char_Received);

  return true;
}

/**
 * called when peripheral disconnects
 */
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Pheripheral forced disconnection, central: ");
  Serial.println(central.address());
  WaitToReconnect();    // in sketch
}

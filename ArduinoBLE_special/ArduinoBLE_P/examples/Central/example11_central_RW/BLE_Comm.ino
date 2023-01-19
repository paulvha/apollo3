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
const char BLE_PERIPHERAL_NAME[] = "Artemis EXCHANGE BLE";

// UUID's
#define SERVICE "9e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_R_BT_UUID  "9e400002-b5a3-f393-e0a9-e50e24dcca9e" // writing for us (Tx_b_Char)
#define CHARACTERISTIC_R_STR_UUID "9e400003-b5a3-f393-e0a9-e50e24dcca9e" // writing for us (Tx_s_Char)
#define CHARACTERISTIC_W_BT_UUID  "9e400004-b5a3-f393-e0a9-e50e24dcca9e" // reading for us (Rx_b_Char)
#define CHARACTERISTIC_W_STR_UUID "9e400005-b5a3-f393-e0a9-e50e24dcca9e" // reading for us (Rx_s_Char)

// handles
BLECharacteristic Rx_s_Char;
BLECharacteristic Tx_s_Char;
BLECharacteristic Rx_b_Char;
BLECharacteristic Tx_b_Char;
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
 * Receive handle function
 * 
 * @param value : pointer to buffer to store
 * @param vlen  : max length to store
 * @param bin   : Binary characteristic or Ascii String
 * 
 * @ return     : length of bytes read
 * 
 */
int RXData(uint8_t *value, int vlen, bool bin) {
  int lenc;
  
  if (bin) lenc = Rx_b_Char.readValue(value, vlen);
  else     lenc = Rx_s_Char.readValue(value, vlen);
  return(lenc);
}

/**
 * send handle function
 * 
 * @param value : pointer to buffer to send
 * @param vlen  : max length to send
 * @param bin   : Binary characteristic or Ascii String
 * 
 * @ return     : length of bytes read
 * 
 */
bool TXData(uint8_t *value, int vlen, bool bin) {

  if (bin)  Tx_b_Char.writeValue(value,  vlen);
  else      Tx_s_Char.writeValue(value,  vlen);
  return(true);
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
  Rx_b_Char = peripheral.characteristic(CHARACTERISTIC_W_BT_UUID);
  Tx_b_Char = peripheral.characteristic(CHARACTERISTIC_R_BT_UUID);
  Rx_s_Char = peripheral.characteristic(CHARACTERISTIC_W_STR_UUID);
  Tx_s_Char = peripheral.characteristic(CHARACTERISTIC_R_STR_UUID);

  if (!Rx_b_Char || !Tx_b_Char || !Rx_s_Char || !Tx_s_Char ) {
    SERIAL_PORT.println(F("Peripheral does not have all characteristics!\r"));
    perform_disconnect();
    return false;
  }
  else if (!Tx_b_Char.canWrite() || !Tx_s_Char.canWrite() ) {
    SERIAL_PORT.println(F("\rPeripheral does not have a writable characteristic!\r"));
    perform_disconnect();
    return false;
  }

  if (!Rx_b_Char.canRead() ||!Rx_s_Char.canRead()  ) {
    SERIAL_PORT.println(F("\rPeripheral does not have a readable characteristic!\r"));
    peripheral.disconnect();
    return false;
  }

  SERIAL_PORT.println(F("Discovered\r"));
  
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

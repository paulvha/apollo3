/*
 * This file contains the routines that bridge between user level in the sketch
 * and the amdtp - protocol handler layer
 * 
 * Version 3.0 / October 2020 / paulvha
 *  initial version
 *  
 */
 
#include "ArduinoBLE.h"
#include "amdtp_common.h"
#include "amdtp_bridge.h"

uint8_t ReceivedBuf[MAXRECEIVEBUF]; // encode
uint16_t ReceivedLen;               // encode

// keep track of connection status
uint8_t AMD_stat = AMD_IDLE;

BLECharacteristic RxChar;
BLECharacteristic TxChar;
BLECharacteristic AckChar;
BLEDevice peripheral;
bool LastPacketSendWasData;       // problem with Notify in ArduinoBLE

//****************************************
//
// Receive / send functions
//
//****************************************

void RxChar_Received(BLEDevice central, BLECharacteristic characteristic) {
  
#ifdef BLE_SHOW_DATA 
  Serial.print(F("\rRX receive "));
#endif

  String a = (const char*) characteristic.value();
  int vsize = characteristic.valueSize();
  encode_receipt(a,vsize);
  
  AmdtpReceivePkt(AMDTP_PKT_TYPE_DATA, ReceivedLen, ReceivedBuf);
}

// not expected to receive anything on the TX characteristic
void TxChar_Received(BLEDevice central, BLECharacteristic characteristic) {
  
#ifdef BLE_SHOW_DATA
  Serial.print(F("\rTX receive "));
#endif

  String a = (const char*) characteristic.value();
  int vsize = characteristic.valueSize();
  encode_receipt(a,vsize);
}

// send data packet over the TX characteristic
void TxChar_Write(uint8_t *value, uint16_t vlen) {
  
#ifdef BLE_Debug
  Serial.println(F("\rTX / data packet write"));
#endif
  
  LastPacketSendWasData = true;
  
  TxChar.writeValue(value, (int) vlen);
}

void AckChar_Received(BLEDevice central, BLECharacteristic characteristic) {
  
#ifdef BLE_SHOW_DATA
  Serial.print(F("\rACK receive"));
#endif
  
  // ArduinoBLE Ack is returning the same package as sent by this client
  // causes the stack to go crazy. It is actually an error in ArduinoBLE.
  // We now ignore if we did not sent a Data package just before
  if (! LastPacketSendWasData) {
    
#ifdef BLE_SHOW_DATA
  Serial.println(F("d Ignore package !! (Notify of send Ack)"));
#endif    
    return;
  }

  String a = (const char*) characteristic.value();
  int vsize = characteristic.valueSize();
  encode_receipt(a,vsize);
  AmdtpReceivePkt(AMDTP_PKT_TYPE_ACK, ReceivedLen, ReceivedBuf);
}

// send data packet over the ACK characteristic
void AckChar_Write(uint8_t *value, uint16_t vlen) {
  
#ifdef BLE_Debug
  Serial.println(F("\rAck write:"));
#endif

  LastPacketSendWasData = false;
  
  AckChar.writeValue(value, (int) vlen);
}

/*
 * As we receive bytes on a String characteristic, we can not have a zero in the middle of the 
 * bytes to be received. As the sender side we translate a zero to 0x73 0x20. This routine
 * will just do the opposite and translate 0x7e 0x20 back to 0x0. THis is done in gatt.c on 
 * Linux/ Ubuntu.
 * 
 * Hope and pray that 0x7e 0x20 strings do not happen ... else we extend the decode one morestep.
 */
void encode_receipt(String a, int vsize)
{
  char save = 0x0;
  ReceivedLen = 0;

  for (int i = 0; i < vsize; i++) {

    char c = a[i];
    
    // Header character for 0x0 ??
    if (c == 0x7E && save == 0x0) {
        save = 0x7E;
        continue;
    }
    
    // did we get a header character before
    if (save == 0x7E) {
      
      // if next is 0x20 then the real character was 0x0;
      if (c == 0x20)  c = 0x0;

      // the previous 0x7E was not header, add now
      else {
        ReceivedBuf[ReceivedLen++] = save;
      }

    save = 0x0; // reset header
    }

    // Store real current character
    ReceivedBuf[ReceivedLen++] = c;
  }
  
#ifdef BLE_SHOW_DATA
  
  Serial.printf("\r\nRaw received (length %d) : ",vsize);
  for (int i = 0; i < vsize; i++) {
    Serial.print(" 0x");  Serial.print(a[i],HEX);
  }
  
  Serial.printf("\r\nAfter encode (size %d) :", ReceivedLen);
  for (size_t i = 0; i < ReceivedLen; i++) {
    Serial.print(" 0x"); Serial.print(ReceivedBuf[i],HEX);
  }

  Serial.println();
#endif
}

//****************************************
//
// Bluetooth functions
//
//****************************************
/* initialize ArduinoBLE */
bool StartBLE()
{
  AMD_stat = AMD_IDLE;
  
  // BLE.debug(Serial); // enable display HCI commands
    
  // begin initialization
  if (!BLE.begin()) return false;

  return true;
}

/* perform poll for any BLE events and check for connected */
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

/* perform a disconnect */
void perform_disconnect()
{
  if (AMD_stat == AMD_CONNECTED) BLE.disconnect();
  while (CheckConnect());
}

/* scan for AMDTP server*/
bool PerformScan()
{
  if (AMD_stat != AMD_IDLE) return false;
  
  Serial.println("\rScan for AMDTP service");

  unsigned long st = millis();
  
  // start scanning for AMDTP peripheral
  BLE.scanForUuid(ATT_UUID_AMDTP_SERVICE);
  
  while(1) {
    
    // check if a peripheral has been discovered
    peripheral = BLE.available();
  
    if (peripheral) {
      
      // discovered a peripheral
      Serial.println("\rDiscovered an AMDTP peripheral");
      Serial.println("\r-----------------------");
  
      // print address
      Serial.print("\rAddress: ");
      Serial.println(peripheral.address());
      
#ifdef BLE_Debug     
      // print the local name, if present
      if (peripheral.hasLocalName()) {
        Serial.print("\rLocal Name: ");
        Serial.println(peripheral.localName());
      }
  
      // print the advertised service UUIDs, if present
      if (peripheral.hasAdvertisedServiceUuid()) {
        Serial.print("\rService UUIDs: ");
        for (int i = 0; i < peripheral.advertisedServiceUuidCount(); i++) {
          Serial.print(peripheral.advertisedServiceUuid(i));
          Serial.print(" ");
        }
        Serial.println();
      }
  
      // print the RSSI
      Serial.print("\rRSSI: ");
      Serial.println(peripheral.rssi());
  
      Serial.println();
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

/* connect and obtain handles */
bool get_handles()
{
  if (AMD_stat != AMD_SCANNED) return false; 
  
  // connect to the peripheral
  Serial.println("\rConnecting ...");

  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return false;
  }

  AMD_stat = AMD_CONNECTED;
  
  // discover peripheral attributes
  Serial.print("\rDiscovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    perform_disconnect();
    return false;
  }

  // retrieve the characteristics
  RxChar = peripheral.characteristic(ATT_UUID_AMDTP_TX);
  TxChar = peripheral.characteristic(ATT_UUID_AMDTP_RX);
  AckChar = peripheral.characteristic(ATT_UUID_AMDTP_ACK);
  
  if (!RxChar || !TxChar || !AckChar ) {
    Serial.println("\rPeripheral does not have all characteristics!");
    perform_disconnect();
    return false;
  } else if (!TxChar.canWrite() || !AckChar.canWrite()  ) {
    Serial.println("\rPeripheral does not have a writable characteristic!");
    perform_disconnect();
    return false;
  } else if (!RxChar.canSubscribe() || !AckChar.canSubscribe()  ) {
    Serial.println("\rPeripheral does not have a subscribe characteristic!");
    perform_disconnect();
    return false;
  }

  if (!RxChar.subscribe()) {
    Serial.println("\rRxCharacteristic notify subscription failed!");
    peripheral.disconnect();
    return false;
  }

  if (!AckChar.subscribe()) {
    Serial.println("\rAck-Characteristic notify subscription failed!");
    peripheral.disconnect();
    return false;
  }
  
  // assign event handlers for characteristic
  RxChar.setEventHandler(BLEUpdated, RxChar_Received);
  TxChar.setEventHandler(BLEUpdated, TxChar_Received);    // should never happen..
  AckChar.setEventHandler(BLEUpdated, AckChar_Received);

  return true;
}

//****************************************
//
// C-callable functions
//
// ****************************************
extern "C" void HandeRespServer(uint8_t * buf, uint16_t len) {
  HandleServerResp(buf, len);
}

extern "C" void SendDataPacket(uint8_t *value, uint16_t vlen) {
  TxChar_Write(value, vlen);
}

extern "C" void SendAckPacket(uint8_t *value, uint16_t vlen) {
  AckChar_Write(value, vlen);
}
extern void set_led_high( void ){
  digitalWrite(LED_BUILTIN, HIGH);
}

extern void set_led_low( void ){
  digitalWrite(LED_BUILTIN, LOW);
}

// ****************************************
//
// Debug print functions
//
// ****************************************
#if (defined BLE_Debug) || (defined BLE_SHOW_DATA)  //// amdtp_common.h

#define DEBUG_UART_BUF_LEN 256

char debug_buffer [DEBUG_UART_BUF_LEN];

extern "C" void debug_float (float f) {
  SERIAL_PORT.print(f);
}

extern "C" void debug_print(const char* f, const char* F, uint16_t L){
  SERIAL_PORT.printf("\rfm: %s, file: %s, line: %d\n", f, F, L);
}

extern "C" void debug_printf(char* fmt, ...){
    va_list args;
    va_start (args, fmt);
    vsnprintf(debug_buffer, DEBUG_UART_BUF_LEN, (const char*)fmt, args);
    va_end (args);

    SERIAL_PORT.print(debug_buffer);
}
#else  // do nothing
extern "C" void debug_printf(char* fmt, ...){}

#endif //BLE_Debug & BLE_SHOW_DATA

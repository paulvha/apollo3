/*
 * This file contains the routines that bridge between user level  in the sketch
 * and the amdtp - protocol handler layer
 * 
 * Version 3.0 / October 2020 / paulvha
 *  initial version
 *  
 */

#include "amdtp_common.h"
#include "amdtp_bridge.h"

uint8_t ReceivedBuf[100]; // encode
uint16_t ReceivedLen;     // encode
String s_value;           // sending

void blePeripheralConnectHandler(BLEDevice central) {

  Serial.print("\rConnected event, central: ");
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLEDevice central) {

  Serial.print("\rDisconnected event, central: ");
  Serial.println(central.address());
  
  // reset all pointers
  amdtps_init();
}

void RxChar_Received(BLEDevice central, BLECharacteristic characteristic) {
  
#ifdef BLE_SHOW_DATA 
  Serial.print(F("\r\nRX receive "));
#endif

  String a = (const char*) characteristic.value();
  int vsize = a.length();
  encode_receipt(a,vsize);
  
  AmdtpReceivePkt(AMDTP_PKT_TYPE_DATA, ReceivedLen, ReceivedBuf);
}

// not expect to receive anything on the TX characteristic
void TxChar_Received(BLEDevice central, BLECharacteristic characteristic) {
  
#ifdef BLE_SHOW_DATA
  Serial.print(F("\r\nTX receive "));
#endif

  String a = (const char*) characteristic.value();
  int vsize = a.length();
  encode_receipt(a,vsize);
}

// send data packet over the TX characteristic
void TxChar_Write(uint8_t *value, uint16_t vlen) {
  
#ifdef BLE_Debug
  Serial.print(F("\r\nTX / data packet write: "));
#endif

  PrepSending(value,vlen);
  TxChar.writeValue(s_value);
}

void AckChar_Received(BLEDevice central, BLECharacteristic characteristic) {
  
#ifdef BLE_SHOW_DATA
  Serial.print(F("\r\nACK receive"));
#endif

  String a = (const char*) characteristic.value();
  int vsize = a.length();
  encode_receipt(a,vsize);
  
  AmdtpReceivePkt(AMDTP_PKT_TYPE_ACK, ReceivedLen, ReceivedBuf);
}

void AckChar_Write(uint8_t *value, uint16_t vlen) {
  
#ifdef BLE_Debug
  Serial.print(F("\r\nAck write:"));
#endif

  PrepSending(value,vlen);
  AckChar.writeValue(s_value);
}

// create String to send
void PrepSending(uint8_t *value, uint16_t vlen) {
  
  s_value = "";
  for (size_t i = 0; i < vlen; i++)  s_value += (char) value[i];
  
#ifdef BLE_Debug
  for (size_t i = 0; i < s_value.length(); i++) {
    Serial.print(" 0x"); Serial.print(s_value[i],HEX);
  }
  Serial.println();
#endif
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
  for (int i = 0; i < ReceivedLen; i++) {
    Serial.print(" 0x"); Serial.print(ReceivedBuf[i],HEX);
  }

  Serial.println();
#endif
}

//****************************************
//
// C-callable functions
//
// ****************************************
extern "C" void UserRequestRec(uint8_t * buf, uint16_t len){
  UserRequestReceived(buf, len);
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

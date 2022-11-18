// CHANGE ArduinoBLE_P TO ArduinoBLE WHEN USING VERSION 2.X OF SPARKFUN LIBRARY

#include <ArduinoBLE_P.h>
/*
  This sketch demostrates the sending and receiving of data between 2 BLE devices
  It acts as the pheripheral, thus it will advertise the service and characteristics

  There are 2 characteristics (one for reading, one for writing) for binary data exchange and
  2 characteristics (one for reading, one for writing) for ASCII string exchange.

  In each message received the first byte MUST be the MAGICNUM.

  There are 4 call_backs

  String characteristic written
  Byte characteristic written
  String characteristic being read
  Byte characteristic being read

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  PROBLEM / ERROR

  In first instance I used a single characteristic for String. This was set for BLERead and BLEWrite.
  When this sketch is performs a writeValue() it will set a message with length of 33. As the central
  is reading, the ATT layer of ArduinoBLE will check for a valueLength(). If available it will try to
  obtain that length (33) with readValue().

  Now as the remote / central is performing a writeValue() of a 5 character long value on the same
  characteristic this will be stored. The String_written_callback is done. The valuelength is now 5
  and we can obtain that value.

  HOWEVER....
  Now the central is performing another read, the ATT layer of ArduinoBLE will check for a valueLength().
  , which is NOW 5 !! It will try to obtain that length with readValue(). But from readValue() the
  String_read_callback is called and it will set a value of 33 characters. BUT only 5 will be read
  now.

  Of course we can, after reading what was send, immediately do a write of 33 character string, but
  actually the values from the pheripheral and central should be stored separatly in ArduinoBLE.

  SO YOU CAN ONLY USE BLEREAD AND BLEWRITE ON THE SAME CHARACTERISTIC IF THE LENGTH OF THE VALUES SEND
  BY EITHER PHERIPHERAL OR CENTRAL ARE THE SAME !!

  Actually the values from the pheripheral and central should be stored separatly in ArduinoBLE.

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  OBSERVATION 1
  Of course we can, after reading what was send, immediately do a write of 33 character string.
  You can set a dummy of (in this case) 33 characters and wait for BLERead_call_back to update that
  to the most actual value that you really want to send. That will only work if that latest value is
  of the same size of the dummy (33 characters in our case).

  OBSERVATION 2
  In any case where a remote is trying to read a characteristic, you MUST first set a value. in case of
  using BLERead_call_back the size of that value MUST be the same size of the next value.
  (see the description above for explanation).

  OBSERVATION 3
  You can use notify, but that will only send as bytes that fit in the agreed MTU size between peripheral
  and central after connect. The default size is 23. unfortunatly there is no library call to obtain
  the agreed MTU size.

  OBSERVATION 4
  A way around is to first set a new value with the correct size on the characteristic.
  Then use a NOTIFY characteristic to send SHORT message the remote to indicate that a
  read characteristic is ready to be read.

*/
///////////////////////////////////////////////////////
// BLE defines
///////////////////////////////////////////////////////
const char BLE_PERIPHERAL_NAME[] = "Artemis EXCHANGE BLE";

// create characteristic and allow remote device to read and write
#define CHARACTERISTIC_R_BT_UUID  "9e400002-b5a3-f393-e0a9-e50e24dcca9e" // reading for us
#define CHARACTERISTIC_R_STR_UUID "9e400003-b5a3-f393-e0a9-e50e24dcca9e" // reading for us
#define CHARACTERISTIC_W_BT_UUID  "9e400004-b5a3-f393-e0a9-e50e24dcca9e" // writing for us
#define CHARACTERISTIC_W_STR_UUID "9e400005-b5a3-f393-e0a9-e50e24dcca9e" // writing for us

// create the service
BLEService Service("9e400001-b5a3-f393-e0a9-e50e24dcca9e");

// create characteristics and allow remote device to read, so we write
BLECharacteristic b_w_Characteristic(CHARACTERISTIC_W_BT_UUID, BLERead | BLEWrite, 100);
BLEStringCharacteristic s_w_Characteristic(CHARACTERISTIC_W_STR_UUID, BLERead | BLEWrite,100);

// create characteristics to allw remove devices to write, so we read
BLECharacteristic b_r_Characteristic(CHARACTERISTIC_R_BT_UUID, BLERead | BLEWrite, 100);
BLEStringCharacteristic s_r_Characteristic(CHARACTERISTIC_R_STR_UUID, BLERead | BLEWrite,100);

///////////////////////////////////////////////////////
// Program defines
///////////////////////////////////////////////////////
#define MAGICNUM 0XCF                 // should be byte 0 in new message
bool Onetime = true;
bool Connected = false;
String Test="ABCDEFGHIJKLMNOPQRSTUVW0123456789";
uint8_t TestBinBuf[]={MAGICNUM,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x50};
int8_t t_offset = 0;

///////////////////////////////////////////////////////
// NO NEED FOR CHANGES BEYOND THIS POINT
///////////////////////////////////////////////////////

/**
 * called when BINARY reading is done by remote. so we write
 */
void BinaryReadCallBack(BLEDevice *central, BLECharacteristic c) {
    Serial.println("Binary read call_Back is called!");

    // sendin new binary test data
    b_w_Characteristic.writeValue(TestBinBuf,sizeof(TestBinBuf));
}

/**
 * called when ASCII / STRING reading is done by remote. so we write
 */
void StringReadCallBack(BLEDevice *central, BLECharacteristic c) {
    Serial.println("String read call_Back is called!");
    // send sliding String
    Send_data();
}

/**
 * called when writing is done by remote. so we read
 */
void BinaryWriteCallBack(BLEDevice *central, BLECharacteristic c) {
    Serial.println("Binary write Back is called!");

    int lenc = c.valueLength();
    uint8_t *valuec=(uint8_t*) c.value();

    // check for correct packet start
    if (valuec[0] != MAGICNUM) {
      Serial.print(F("Invalid magic number : 0x"));
      Serial.println(valuec[0],HEX);
      return;
    }

    // skip magic number
    for (int i = 1; i < lenc; i++)
      Serial.print(valuec[i],HEX);

    Serial.println();
}

/**
 * called when ASCII / STRING writing was done by remote. so we read
 */
void StringWriteCallBack(BLEDevice *central, BLECharacteristic c) {
    Serial.println("String write Back is called!");

    int lenc = c.valueLength();
    uint8_t *valuec = (uint8_t*) c.value();

    // check for correct packet start
    if (valuec[0] != MAGICNUM) {
      Serial.print(F("Invalid magic number : 0x"));
      Serial.println(valuec[0],HEX);
      return;
    }

    // skip magic number
    for (int i = 1; i < lenc; i++)
      Serial.print((char) valuec[i]);

    Serial.println();
}

/*
 * send a sliding string
 */
void Send_data(){
  int i,j;
  String t_send;

  j=t_offset++;

  if (t_offset == Test.length()) t_offset=0;

  /* if the message is longer than 100 characters, you
   * could break it in blocks. The second byte of the
   * first message (after the MAGICNUM) could be the
   * total length of all the blocks.
   */
  // add Magic to indicate a valid start
  t_send += (char) MAGICNUM;

  // for each letter
  for (i = 0; i < Test.length();i++){

   // copy from start offset
   t_send += Test[j++];

   // loop around
   if (j == Test.length()) j = 0;
  }

  Serial.print("Sending content: ");
  Serial.println(t_send.substring(1,t_send.length())); // skip magic
  Serial.print("length: ");
  Serial.println(t_send.length()-1);

  s_w_Characteristic.writeValue(t_send);
}

void setup() {
   Serial.begin(115200);
   delay(1000);
   Serial.print("\nExampe11 Peripheral RW. Compiled on: ");
   Serial.println(__TIME__);

  ///////////////////////////////////////////////////
  // BLE SETUP START
  ///////////////////////////////////////////////////
  // begin initialization
  if (!BLE.begin()) {
    Serial.println(F("starting BLE failed!\r"));
    while (1);
  }

  // set the local name peripheral advertises
  BLE.setLocalName(BLE_PERIPHERAL_NAME);

  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(Service);

  // add the characteristics to the service
  Service.addCharacteristic(b_r_Characteristic); // add BYTE for us to read characteristic
  Service.addCharacteristic(b_w_Characteristic); // add BYTE for us to write characteristic
  Service.addCharacteristic(s_r_Characteristic); // add String for us to read characteristic
  Service.addCharacteristic(s_w_Characteristic); // add String for us to write characteristic

  // add the service
  BLE.addService(Service);

  // set connection handlers
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // start advertising
  BLE.advertise();

  // called when read is performed by remote (and thus we write)
  b_w_Characteristic.setEventHandler(BLERead, (BLECharacteristicEventHandler) BinaryReadCallBack);
  s_w_Characteristic.setEventHandler(BLERead, (BLECharacteristicEventHandler) StringReadCallBack);

  // called when something written by remote (and thus we read)
  b_r_Characteristic.setEventHandler(BLEWritten, (BLECharacteristicEventHandler) BinaryWriteCallBack);
  s_r_Characteristic.setEventHandler(BLEWritten, (BLECharacteristicEventHandler) StringWriteCallBack);

  ///////////////////////////////////////////////////
  // BLE SETUP END
  ///////////////////////////////////////////////////

  Serial.println(F("ready to go"));
}

void loop() {

  // wait for connection
  while (! Connected){
    BLE.poll();
    delay(1000);     // don't hammer
  }

  if (Onetime){      // send first time after connect
    delay(1000);     // wait for central to settle
    Send_data();
    b_w_Characteristic.writeValue(TestBinBuf,sizeof(TestBinBuf));
    Onetime = false;
  }

  BLE.poll();
  delay(500);        // don't hammer
}

/**
 * called when central connects
 */
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  Connected = true;
  Onetime = true;
  t_offset=0;
}

/**
 * called when central disconnects
 */
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  Connected = false;      // block loop and wait for new connection
}

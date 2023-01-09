/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   
   
   Ported to ArduinoBLE_P / paulvha

    Version 1.0 / january 2023
    * initial version
   
   Create a BLE peripheral that will send iBeacon frames.
   The design of creating the BLE peripheral is:
   1. Create a BLE peripheral
   2. Create advertising data
   3. Optional add device name to scanresponse
   3. Start advertising.

   The ArduinoBLE does not allow you to set the real TXpower, fast advertising nor 
   selecting specific BLE channels.
      
   If you want to stop advertising / powerdown / sleep for a certain time, you can add this in loop for 
   the processor you have.


   Tested with different Beacon scanners:
   
   On the Iphone "Beacon scan" I had to enter to UUID the  other way around 
   (4D6FC88B-BE75-6698-DA48-6866A36EC78E). Then it found the Beacon but it did not show the name.

   With nrf-scanner on Android it showed the name + iBeacon in the scan windows.

   On Android the "beacon scanner" showed the name in the details. (double click on the screen) 
*/

/*
 * Background
 * ==========
 * 
 * Beacon uses the Manufacturer Specific Data field in the advertising packet, which means 
 * you must provide a valid Manufacturer ID. Update the field below to an appropriate value. 
 * For a list of valid IDs see:
 * https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers
 * 0x004C is Apple
 * 0x0822 is Adafruit
 * 0x0059 is Nordic
 * 
 *
 * -----------------------------------------------------------------------------------------
 * https://en.wikipedia.org/wiki/IBeacon
 * 
 * In order to be recognized as a iBeacon, you need to have a payload like :
 * 
 * 0201061AFF4C0002154D6FC88BBE756698DA486866A36EC78E01200212BD
 * 
 * 0201061AFF4C00  Apple's iBeacon advertising prefix 
 *  02    length next field
 *  01    opcode BLEFieldFlags
 *  06    flags (BLEFlagsBREDRNotSupported | BLEFlagsGeneralDiscoverable)
 *  1A    total length of Beacon (26 bytes)
 *  FF    Flag for manufacturer specific data
 *  4C00  APPLE's manufacturers ID
 * 
 * 02     Apple's iBeacon type of Custom Manufacturer Data
 * 15     length Of the rest of the iBeacon data; UUID + Major + Minor + TXPower)
 * 4D6FC88BBE756698DA486866A36EC78E   UUID
 * 0120   Major
 * 0212   Minor
 * BD     8 bit Signed value, ranges from -128 to 127, use Two's Compliment to "convert" if necessary, 
 *        Units: Measured Transmission Power in dBm @ 1 meters from beacon) (Set by user, not dynamic, 
 *        can be used in conjunction with the received RSSI at a receiver to calculate rough distance 
 *        to beacon) 
 *
 * All together the length is 30 bytes. The maximum an advertisting payload can have is 31 bytes
 * so no space for name, service etc. etc.
 * 
 * But if you want you can add a local name in the scanresponse, which could be displayed by some receivers. 
 */
 
// This header file can be replaced  by ArduinoBLE.h if you have the standard version.
// The ArduinoBLE_P library has a number stability patches that have not bee included (yet) 
// in the official version.(https://github.com/arduino-libraries/ArduinoBLE)
 
#include "ArduinoBLE_P.h"
#include "BLEBeacon.h"

// define you own major and minor number
static uint16_t Major = 0x0120;
static uint16_t Minor = 0x0212; 

// by defining a local name it will be added in the scanresponse
// if not defined no name is set
#define BLE_PERIPHERAL_NAME "Test_Beacon"

// See the following for generating UUIDs:
// UUID 1 128-Bit (may use linux tool uuidgen or random numbers via https://www.uuidgenerator.net/)
#define BEACON_UUID  "8ec76ea3-6668-48da-9866-75be8bc86f4d" 

// Restart advertising after (potential) connect and disconnect
bool RESTART_Advertise = true;

// uncomment to enable BLE debug messages
//#define BLE_Debug 1

////////////////////////////////////////////////////////////////////////////////
//        NO NEED FOR CHANGES BEYOND THIS POINT
////////////////////////////////////////////////////////////////////////////////

static uint8_t ServiceData[30];

void setBeacon() {

  BLEBeacon oBeacon = BLEBeacon();
  oBeacon.setManufacturerId(0x4C00); // fake Apple 0x004C LSB (ENDIAN_CHANGE_U16!)
  oBeacon.setProximityUUID(BLEUUID(BEACON_UUID));
  oBeacon.setMajor(Major);
  oBeacon.setMinor(Minor);
  oBeacon.setSignalPower(-67);      // you can define you own

  int cnt = oBeacon.getDataU(ServiceData, 30); 
  BLE.setManufacturerData((const uint8_t *) ServiceData, cnt);

#ifdef BLE_PERIPHERAL_NAME
  // set the local name peripheral advertises
  BLE.setLocalName(BLE_PERIPHERAL_NAME);
#endif

  // This can be updated, but only if you know what you are doing
  BLE.setAdvertisingInterval(160);        // Min and Max in units of 0.625 ms for interval and window. of 0.625 ms (160 = 10 ms)
}

void setup() {
    
  Serial.begin(115200);
  while(!Serial);

  Serial.print("Start iBeacon ");
  Serial.print(Major,HEX);
  Serial.print("-");
  Serial.println(Minor,HEX);

  // BLE INIT

#ifdef BLE_Debug
  BLE.debug(Serial);         // enable display HCI commands on ArduinoBLE_P
#endif

  // begin initialization
  if (!BLE.begin()) {
    Serial.println(F("Starting BLE failed! Freeze.\r"));
    while (1);
  }

  // set call backs
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
  
  setBeacon();
  
  // Start advertising
  BLE.advertise();
  Serial.println("Advertizing started...");
}

void loop() {
  // here you can add Stop advertising, perform a sleep and restart advertizing
}

/**
 * called when central connects
 */
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  BLE.disconnect();
}

/**
 * called when central disconnects
 */
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());

  if (RESTART_Advertise) {
    BLE.advertise();
    Serial.println("Advertizing started again...");
  }
}

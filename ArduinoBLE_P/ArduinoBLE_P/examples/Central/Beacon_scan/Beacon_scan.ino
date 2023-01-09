/*
  iBeacon scan

  This example scans for BLE peripherals with special attention for (i)Beacons. 
    
  (i)Beacons are peripherals that advertise a special formatted message as defined by Apple. See more inforamation on https://en.wikipedia.org/wiki/IBeacon
  An (i)Beacon is used to find your way around a building or to a specific place.

  This scanner will display the details of a discovered (i)Beacon as well as information about other peripherals.

  Given the speed at which you receive the information it might only be relevant to see the RSSI and determine how far away you are from a (i)Beacon. Thus this
  sketch has a filter function to display the details of an (i)Beacon only one time and from that moment on the UUID and the RSSI. You can disable this filter. (APPLYFILTER)

  You can also select only see the (i)Beacon information or to display info about other peripherals as well. (ONLYBEACONS)

  During my test I had issues with setting a 'scan callback'. With a scan- callback the scanned data is "pushed" to the sketch and caused an overload (and crash) on Artemis /Apollo3
  boards. It worked well without problems with the nrf52840 boards. All has to do with the structure of the library. Hence there is any option to use call back (WITHCALLBACK) which
  will push, or to use a pull structure from loop.


  See include Scan.txt for more information about scan advertised data from peripheral and whether or not to use call Back.
  
  Version 1.0 / January 2023 / paulvha
  * initial version 

  This example code is in the public domain.
*/
// This header file can be replaced  by ArduinoBLE.h if you have the standard version.
// The ArduinoBLE_P library has a number stability patches that have not bee included (yet) 
// in the official version.(https://github.com/arduino-libraries/ArduinoBLE)
#include <ArduinoBLE_P.h>
#include "Manufac_ID.h"

// you can select scan with call back or detect in loop
// some boards will crash with call back due to the amount of traffic
//#define WITHCALLBACK 1

// you can select to only see the Beacon scan result, not other peripherals
#define ONLYBEACONS 1

// you can select to have the built-in led blink as keep alive
#define SHOWKEEPALIVE

// You can apply filter to display Beacon details only once
// else it will show a single line with UUID and RSSI
#define APPLYFILTER

// uncomment to enable BLE debug messages
//#define BLE_Debug 1

///////////////////////////////////////////////////////////////
//   NO NEED FOR CHANGES BEYOND THIS POINT
///////////////////////////////////////////////////////////////

#ifdef APPLYFILTER
// filter for Beacon
#define MaxBeacon 5
uint8_t B_filter[MaxBeacon][16];
uint8_t NumBeacon = 0;      // number Beacon UUID stored
#endif

// global variables
uint8_t addata[63];         // 31 advertisment data + (potential) 31 scan data
int dlen;
int adlen = 0;

#ifdef SHOWKEEPALIVE
#define blinky 3000         // every 3 seconds
#endif

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("BLE iBeacon scan");
  
#ifdef BLE_Debug
  BLE.debug(Serial);         // enable display HCI commands on ArduinoBLE_P
#endif

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }
  
#ifdef WITHCALLBACK
  // set the discovered event handle
  BLE.setEventHandler(BLEDiscovered, bleCentralDiscoverHandler);
#endif

  // start scanning for peripherals with duplicates
  BLE.scan(true);

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {

#ifdef SHOWKEEPALIVE
  static bool stl = false;
  static unsigned long st = 0; 

  // like a keep alive signal
  if (millis() - st > blinky){
    if (stl) {
      digitalWrite(LED_BUILTIN, LOW);
      stl = false;
    }
    else {
      digitalWrite(LED_BUILTIN, HIGH);
      stl = true;
    }

    st = millis();
  }
#endif // SHOWKEEPALIVE

  // poll the central for events
  BLE.poll();

#ifndef WITHCALLBACK
  // check if a peripheral has been discovered
  BLEDevice peripheral = BLE.available();

  if (peripheral) bleCentralDiscoverHandler(peripheral);
#endif
}

/**
 * handle received scan data
 */
void bleCentralDiscoverHandler(BLEDevice peripheral) {
  
  if ( !CheckBeacon(peripheral) ) {

#ifdef ONLYBEACONS
  return;
#else
  
    // discovered a peripheral
    Serial.println("Discovered a peripheral");
    Serial.println("=======================");
  
    // print address
    Serial.print("Address: ");
    Serial.println(peripheral.address());
  
    // print the local name, if present
    if (peripheral.hasLocalName()) {
      Serial.print("Local Name: ");
      Serial.println(peripheral.localName());
    }
  
    // print the advertised service UUIDs, if present
    if (peripheral.hasAdvertisedServiceUuid()) {
      Serial.print("Service UUIDs: ");
      for (int i = 0; i < peripheral.advertisedServiceUuidCount(); i++) {
        Serial.print(peripheral.advertisedServiceUuid(i));
        Serial.print(" ");
      }
      Serial.println();
    }
  
    // print the RSSI
    Serial.print("RSSI: ");
    Serial.println(peripheral.rssi());
#endif // ONLYBEACONS
  }

  Serial.println();

}

/**
 * check and display in case of beacon
 */
bool CheckBeacon(BLEDevice peripheral) {

  dlen = peripheral.advertisementDataLength();
  uint8_t i, j, k;
  uint8_t t[]{4,2,2,2,6}; // where to place '-'

  if (dlen == 0 ) return(false);

  if (dlen > 65) {
    Serial.println("WARNING : Truncating data");
    dlen = 65;
  }

  int adlen = peripheral.advertisementData(addata,dlen);

#if 0   // debug only
    for (i=0; i < adlen ; i++){
      Serial.print(addata[i], HEX);
      Serial.print(" ");
      Serial.println();
    }
#endif

  // check for Beacon header
  //            length         flags opcode              length               manufacturer         subtype                   length
  if (addata[0] == 0x2 && addata[1] == 0x1 && addata[3] == 0x1A && addata[4] == 0xFF && addata[7] == 0x02 && addata[8] == 0x15 ){

    uint16_t man = addata[6]<<8 | addata[5];
    
    // check whether seen before (true = short message)
    if (FilterBeacon()) {

      if (man == 0x004C) Serial.print("IBeacon UUID: ");    // Apple
      else Serial.print("Beacon UUID: ");
    
      // display UUID
      for (i = 9, j=0, k = 0 ; i < (9+16) ; i++,j++) {
        Serial.print(addata[i], HEX);
        
        if (j == t[k]){   // add dashes on the right place
          Serial.print("-");
          k++;
          j = 0;
        }
      }
      Serial.print(" \t");
      display_RSSI(peripheral);
      
      return true;  // beacon
    }
  
    if (man == 0x004C) Serial.println("Discovered a IBeacon");    // Apple
    else Serial.println("Discovered a Beacon");
    Serial.println("====================");

    Serial.println("////// Advertising info \\\\\\\\\\"); 
    Serial.print("Flags: 0x");
    Serial.print(addata[2],HEX);
    if (addata[2] & BLEFlagsBREDRNotSupported ) Serial.print(" BREDRNotSupported ");
    if (addata[2] & BLEFlagsGeneralDiscoverable) Serial.print(" GeneralDiscoverable");
    if (addata[2] & BLEFlagsLimitedDiscoverable) Serial.print(" LimitedDiscoverable");
  
    // look up manufacturer
    Serial.print("\nManufacturer (0x00");
    Serial.print(man,HEX);
    Serial.print("): ");
    
    for (i = 0 ; i < NumMan ; i++) {
      if (man == m[i].id) {
        Serial.println(m[i].name);
        break; 
      }
    }
  
    if (i == NumMan) Serial.println("Not known");
  
    // display UUID
    Serial.print("UUID : ");
    
    for (i = 9, j=0, k = 0 ; i < (9+16) ; i++,j++) {
      Serial.print(addata[i], HEX);
      
      if (j == t[k]){   // add dashes on the right place
        Serial.print("-");
        k++;
        j = 0;
      }
    }
  
    // display Major / Minor
    uint16_t mm = addata[25] <<8 | addata[26];
    Serial.print("\nMajor: 0x");
    Serial.print(mm, HEX);
    Serial.print(" dec. ");
    Serial.println(mm);
  
    mm = addata[27] <<8 | addata[28];
    Serial.print("Minor: 0x");
    Serial.print(mm, HEX);
    Serial.print(" dec. ");
    Serial.println(mm);
       
    // display RSSI set
    display_RSSI(peripheral);
  
    uint8_t exlen = 29;
  
    // check for (optional) extra information from scanresponse
  
    if (adlen > exlen)
      Serial.println("////// Scan response info \\\\\\\\\\");
    
    while (adlen > exlen++) {
      uint8_t nlen = addata[exlen];
      uint8_t ncode = addata[exlen+1];
  
      // translate Opcode to name
      for (i = 0 ; i < Numff; i++) {
        if (ncode == ff[i].id) {
          Serial.print(ff[i].name);
          Serial.print(": ");
          break; 
        }
      }
  
      if (i == Numff) {
        Serial.print(ncode,HEX);
        Serial.print(" Not known: ");
      }
    
      // Could we store all the data 
      if( exlen + nlen > dlen) {
        Serial.println(" Not enough data available\n");
        return(true);
      }
      // skip len + opcode
      exlen +=2;
      
      // show extra data
      for (i = 0 ; i < nlen-1 ; i++) {
        
        if (ncode == 0x8 || ncode == 0x9) Serial.print((char) addata[exlen++]);
        else Serial.print(addata[exlen++], HEX);
        
        Serial.print(" ");
      }
      Serial.println();
      
    }
    
    return(true);   // beacon
  }
  
  return(false);    // not a Beacon
}

/**
 * check whether we have seen this Beacon UUID before
 * 
 * return :
 *  true : seen before
 *  false: not seen before, added to filter list
 */
bool FilterBeacon() {

#ifndef APPLYFILTER 
  return false;
#else 
  
  uint8_t i,j; 
  
  // check filter
  for(i = 0; i < NumBeacon; i++){
    
    for (j=0 ; j< 16; j++){
      if (B_filter[i][j] != addata[9+j])
        break;
    }

    // match found    
    if (j == 16) return true;
  }

  // add Beacon UUID to filter
  for(j=0 ; j< 16; j++) B_filter[NumBeacon][j] = addata[9+j];
 
  // set entry for next Beacon, else overwrite the last
  NumBeacon++;
  if (NumBeacon == MaxBeacon) NumBeacon--;

  return false;
#endif  // APPLYFILTER
}

/**
 * display RSSI information
 */
void display_RSSI(BLEDevice peripheral)
{
    int ad_rssi = addata[29];
    if (ad_rssi & 0x80) ad_rssi -= 256;
    int rc_rssi = peripheral.rssi();
    float diff;
    
    // assume difference is 20dbM per meter
    // further away
    if (ad_rssi > rc_rssi){
        diff = ad_rssi -rc_rssi;
        diff = diff * 20;
    }
    else {    // close by
      diff = rc_rssi - ad_rssi;
      diff = diff / 20;
    }
  
    Serial.print("Advertised RSSI: ");
    Serial.print(ad_rssi);
    Serial.print(", Real RSSI: ");
    Serial.print(rc_rssi);
    Serial.print(", est. distance: ");
    Serial.print(diff / 100);
    Serial.println("m");
}

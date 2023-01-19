/*
  MTU size
  
  This example will show how check and increase the MTU size. If you want to better understand
  the impact look at https://punchthrough.com/maximizing-ble-throughput-part-2-use-larger-att-mtu-2/

  Many modern processor chips support BLE with integrated HCI-layer. The layers above (L2CAP, ATT, GP, GATT 
  etc) are handled by software / drivers. The default MTU size is 23, this is the minimum by standard.  
  BUT it could be increased. Especially when you do transfer of large data this can be interesting.
  
  The maximum MTU size a peripheral can handle is determined during BLE.begin(). From here the HCI-layer 
  the readLeBufferSize() is called. This function obtains the maximum packet length and maximum # packets 
  that can be queued in the HCI layer. From that packetlength it then subtracts 9 bytes for the ACL header
  to set for the maximum content / data.

  ACL header format:
  
  uint8_t pktType;
  uint16_t handle;
  uint16_t dlen;
  uint16_t plen;
  uint16_t cid;

  Within that MTU also the ATT header needs to fit : 1 byte used for the opcode and another 2 bytes used
  for the characteristic handle. As such the data portion for the MTU we need to subtract another 3 bytes.
   
  An MTU size increase can be agreed between peripheral and central after connect. ONLY the central / client
  should perform that request. However the ArduinoBLE library does not allow you to directly do that.
  
  In ArduinoBLE the call discoverAttributes() will perform that following functions :
      Request to remote to increase to it's max. MTU size
      Discover Services (with of without UUID filter)
      Discover characteristics (with of without UUID filter)
      Discover Descriptors (with of without UUID filter)

  A client/ central has a maximum MTU, but so has the peripheral / server. As part of the protocol to 
  agree on a different MTU-size, the client / central requests an MTU-size up to it's maximum, the 
  peripheral / server will respond with agree or lower size that depending on it's maximum. 
  That agreed MTU size will be used. BUT what is it ???
  
  Especially for applications (sketch) level where you are sending a large complete message broken down in 
  blocks this could be interesting.

  In many implementations of the BLE Stack there is no standard function to obtain the agreed MTU, same with ArduinoBLE. 
  Hence ArduinoBLE_P (P = Patched) has been enhanced to provide that information. 
  The call readMTU() will provide the agreed MTU-size MINUS 3 octects ATT overhead. This example shows how that works.
  
  This sketch is planned to be used with peripheral example13_ph_MTU_size
  This example connects to a BLE peripheral with the standard battery service and level characteristic. 

  However you can specify a maximum characteristic valuesize of 512 ?
  
  What now ? 
  
  In the standard ArduinoBLE V1.2.3 version, the peripheral will send the data up to the agreed MTU-size.  
  It is now down to the central to ask for the next ‘blob’ until it has read all the bytes. 
  However if your central /client is also build with ArduinoBLE, it will not do that. 
  The rest is neglected, not asked from the central, not send, disgarded, neglected ! 
  
  OHOH !
  
  BUT… you are lucky to have the ArduinoBLE_P (P = patched). In the ATT layer it tries to include as much 
  as possible bytes to send in the each MTU-size. The central / client will read / receive the packet. 
  If that is less than the expected amount of bytes in the central it will ask for next ‘blob’ starting 
  with an offset from previous received bytes.  
  
  However, no matter what, if your characteristic uses notify to send the data.  
  Whatever fits in the MTU-size will be send. The rest is neglected. 
  Sorry, but you can only send notifications up to MTU size !!
  
  This example code is in the public domain.

  Version 1.0 / November 2022 / paulvha
*/
// this example ONLY works on the ArduinoBLE_P as it has readMTU()
// see in the doc-folder the file changesToArduinoBLE

#include <ArduinoBLE_P.h>

#define SERVICE       "180f"
#define DEVICENAME    "BatteryMonitor"
#define WAITTIMEOUT   10000
#define MAGICNUM      0xcf

BLEDevice peripheral;
uint8_t MtuSize = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Example13 BLE Central - MTU");
  
  // initialize the BLE hardware
  BLE.begin();

  // look for the peripheral
  ScanForPeri();
  
  if (! ConnectDiscover()) {
    Serial.println("Failed to connect / discover. Freeze\n");
    while(1);
  }
}

void loop() {

  if (! peripheral.connected()) {
    Serial.println("Peripheral Disconnected");
    ScanForPeri();
    if (! ConnectDiscover()) {
      Serial.println("Failed to connect / discover. Freeze\n");
      while(1);
    }
  }

  // let notify happening
  delay(200);
  BLE.poll();
}

/**
 * Scan and look forward device.
 */
bool ScanForPeri()
{
  // start scanning for peripherals
  BLE.scanForUuid(SERVICE);
  
  unsigned long st = millis();
  
  while ( millis() - st < WAITTIMEOUT )
  {
    // check if a peripheral has been discovered
    peripheral = BLE.available();
  
    if (peripheral) {
      // discovered a peripheral, print out address, local name, and advertised service
      Serial.print("Found ");
      Serial.print(peripheral.address());
      Serial.print(" '");
      Serial.print(peripheral.localName());
      Serial.print("' ");
      Serial.print(peripheral.advertisedServiceUuid());
      Serial.println();

      if (peripheral.localName().c_str() != DEVICENAME) {
        // stop scanning
        BLE.stopScan();
        return(true);
      }
    }
  }
  
  // Not Found, stop scanning
  BLE.stopScan();
  
  Serial.print("ERROR: Could not find peripheral ");
  Serial.print(DEVICENAME);
  Serial.println("freeze");
  while(1);    
}

/**
 * connect and discover
 */
bool ConnectDiscover() {
  // connect to the peripheral
  Serial.println("Connecting ...");

  if (peripheral.connect()) {
    Serial.print("Just connected with MTU size :");
    Serial.println(peripheral.readMTU());
  } else {
    Serial.println("Failed to connect!");
    return(false);
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return(false);
  }

  // Now read again to see what has been agreed as the new MTU-size.
  MtuSize = (uint8_t) peripheral.readMTU();
  Serial.print("After discoverAttributes() the new MTU size :");
  Serial.println(MtuSize);
  
  // retrieve the battery characteristic
  BLECharacteristic BatteryLevelChar = peripheral.characteristic("2A19");

  if (!BatteryLevelChar) {
    Serial.println("Peripheral does not have BatteryLevelChar characteristic!");
    peripheral.disconnect();
    return(false);
  } else if (!BatteryLevelChar.canSubscribe()) {
    Serial.println("Peripheral does not have a Notify characteristic!");
    peripheral.disconnect();
    return(false);
  }

  if (!BatteryLevelChar.subscribe()) {
    Serial.println(F("Set notify subscription failed!\r"));
    peripheral.disconnect();
    return(false);
  }
 
  BatteryLevelChar.setEventHandler(BLEUpdated, RxChar_Received);
  
  return(true);
}

/**
 * called when notify handle is updated
 * 
 * data format
    d[0] = MAGICNUM;
    d[1] = MTUSize;
    d[2] = BatteryPerc;
 */
void RxChar_Received(BLEDevice central, BLECharacteristic characteristic) {
  const uint8_t *buf = characteristic.value();
  uint16_t len = characteristic.valueSize();

  if (len != 3 || buf[0] != MAGICNUM) {
    Serial.println("Invalid packet");
    return;
  }

  if(MtuSize != buf[1]) {
    Serial.println("MTU mismatch");
  }
  
  Serial.print("Simulated battery level ");
  Serial.print(buf[2]);
  Serial.println("%%");
  
  //for (uint16_t i = 0; i < len; i++) Serial.println(buf[i]); 
}

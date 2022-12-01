/*
  How to increase Maximum Transmission Unit (MTU)

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
  should perform that request. 
  
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
  
  This example creates a BLE peripheral with the standard battery service and level characteristic. 
  A counter is used to simulate the battery level.

  Upon connect you will first see an data-MTU size of 20. Once connected  central / cleint has called discoverAttributes() 
  The new data-MTU-szie is 239. The printed information should look like this:
  
    Bluetooth device active, waiting for connections...
    Just connected with MTU size : 20
    New MTU size : 239
    written 20
    written 21 
    ....
  
  You can use a generic BLE central app, like LightBlue (iOS and Android) or nRF Connect (Android), 
  to interact with the services and characteristics created in this sketch.

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

// magicnum first byte in package
#define MAGICNUM 0xCF

 // BLE Battery Service
BLEService batteryService("180F");

// BLE Battery Level Characteristic
// remote clients will be able to get notifications if this characteristic changes.
BLECharacteristic batteryLevelChar("2A19", BLERead | BLENotify, 5); 

// data to send
int BatteryPerc = 20; 

// MTU size
uint8_t MTUSize;

void setup() {
  Serial.begin(115200);    // initialize serial communication
  while (!Serial);

  Serial.println("Example13 BLE peripheral - MTU");
  
  pinMode(LED_BUILTIN, OUTPUT); // initialize the built-in LED pin to indicate when a central is connected
  digitalWrite(LED_BUILTIN, LOW);
  
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!. Freeze");
    while (1);
  }

  /* Set a local name for the BLE device
     This name will appear in advertising packets
     and can be used by remote devices to identify this BLE device
     The name can be changed but maybe be truncated based on space left in advertisement packet
  */
  BLE.setLocalName("BatteryMonitor");
  BLE.setAdvertisedService(batteryService);           // add the service UUID
  batteryService.addCharacteristic(batteryLevelChar); // add the battery level characteristic
  BLE.addService(batteryService);                     // add the battery service

  /* Start advertising BLE.  It will start continuously transmitting BLE
     advertising packets and will be visible to remote BLE central devices
     until it receives a new connection */

  // start advertising
  BLE.advertise();

  // call back on connect
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);

  Serial.println("Bluetooth device active, waiting for connections...");
}

void loop() {
  uint8_t d[3];
    
  // wait for a BLE central
  BLEDevice central = BLE.central();

  if (central && central.connected()) {

    if (MTUSize != (uint8_t) central.readMTU())
    {
      MTUSize = (uint8_t) central.readMTU();
      Serial.printf("New MTU size %d\n", MTUSize);
    }
    
    d[0] = MAGICNUM;
    d[1] = MTUSize;
    d[2] = BatteryPerc;
    batteryLevelChar.writeValue(d,3);
    
    Serial.printf("written Battery Percentage %d%%\n",BatteryPerc);
    
    // now just delay for 100 * 20 = 2000ms = 2 seconds (we don't want to hammer)
    // BUT you have to trigger the BLE stack.
    for (int cnt = 0; cnt < 100; cnt++){
      delay(20);
      BLE.poll();
    }
    
    // increase the simulated battery level upto 91%
    if (BatteryPerc++ > 90) BatteryPerc = 20;
   }

  if (central && !central.connected()) {
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
    digitalWrite(LED_BUILTIN, LOW);
    BLE.advertise();
  }
  else {
    delay(200);
    BLE.poll();
  }
}

/**
 * called when central connects
 */
void blePeripheralConnectHandler(BLEDevice central)
{
  BLEDevice Central = central;
  
  digitalWrite(LED_BUILTIN, HIGH);
  
  // first read the default MTU size
  MTUSize = (uint8_t) central.readMTU();
  Serial.print("Just connected with MTU size :");
  Serial.println(MTUSize);

  BLE.poll();
}

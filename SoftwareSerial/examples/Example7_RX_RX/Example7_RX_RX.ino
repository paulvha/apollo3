/*
  Author: Paul van Haastrecht
  Created: October 15 2021
  License: MIT. See SparkFun Arduino Apollo3 Project for more information

  Feel like supporting open source hardware? Buy a board from SparkFun!
  https://www.sparkfun.com/artemis

  This example shows how to send characters at BAUDRATE- bps. 
  Any pin can be used for TX or RX.

  Note: In this SoftwareSerial library you cannot TX and RX at the same time.
  TX gets priority. So if Artemis is receiving a string of characters
  and you do a Serial.print() the print will begin immediately and any additional
  RX characters will be lost.

  Hardware Connections:
  Attach a USB to serial converter (https://www.sparkfun.com/products/15096)
  Connect
    GND on SerialBasic <-> GND on Artemis
    RXO on SerialBasic <-> Pin 8 on Artemis
    TXO on SerialBasic <-> Pin 9 on Artemis
  Load this code
  Open Arduino serial monitor at 115200
  Open Terminal window (TeraTerm) at BAUDRATE
  Press a button in terminal window, you should see it in Arduino monitor
  as well as the otherway around
*/

// define the software serial speed. should not be higher than 19200 for stable connection
#define BAUDRATE 19200

// define PINS Any pins can be used
#define RXpin D8
#define TXpin D9

#include <SoftwareSerial.h>

SoftwareSerial mySerial(RXpin, TXpin); //RX, TX

void setup() {

  Serial.begin(115200);
  Serial.println("Software Serial exchange example");
  Serial.print("Baudrate set at ");
  Serial.println(BAUDRATE);
  
  mySerial.begin(BAUDRATE);

  // give Softserial time to settle
  delay(100);
  
  // flush anything pending
  while (mySerial.available()) mySerial.read();
}

void loop() {

  byte incoming;

  while (mySerial.available())
  {
    incoming = mySerial.read();
    Serial.write(incoming);
    //Serial.print(incoming, HEX);
  }
    
  while (Serial.available())
  {
    incoming = Serial.read();
    mySerial.write(incoming);
  }

  delay(100); //Small delay between prints so that we can detect incoming chars if any
}

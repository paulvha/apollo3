/*
  Author: Paul van Haastrecht
  Created: May 11 2023
  License: MIT. See SparkFun Arduino Apollo3 Project for more information

  Feel like supporting open source hardware? Buy a board from SparkFun!
  https://www.sparkfun.com/artemis

  PURPOSE:
  =======
  This example shows how to fine tune the sending baudrate. 

  BACKGROUND:
  ==========
  This SoftwareSerial implementation includes a possibility to perform compensation on the calculated timing
  as internal routines takes time. It is hard coded in the SoftwareSerial library, but starting with this 
  May 2023 version, I have included routines so you have a better way to fine tune.

  Not every Artemis board is the same. Components (e.g. Xtal) are still within the specifications, 
  but maybe just on the edge. The software version / compiler might be just a little different, 
  the software may include drivers or other routines etc. etc. 

  Based on the baudrate setting it will calculate the expected time to send a byte. That is compared against
  a measured time in the driver. If the time to send is lower, the baudrate is too high so we need less 
  compensation. If the time to send is higher, the baudrate is too low and so we need more compensation.

  The program will display the timing and the compensation being applied 100 times, but it will NOT store the 
  proposed compensation.After every 10 checks it will display the compensation numbers used like :

  Compensation results after 83 checks:
  Compensation 6, used 74 time(s)
  Compensation 7, used 7 time(s)
  Compensation 8, used 1 time(s)
  Compensation 9, used 1 time(s)
    .........................
    
  You can now use this value as part of begin() in the different sketches:

  void begin(uint32_t baudRate, int16_t comp = 0);
    MySerial.begin(9600, 4);     // speed 9600, compensation 4
    
  void begin(uint32_t baudRate, uint16_t SSconfig, int16_t comp = 0 );
    mySerial.begin(19200, SERIAL_8E2, 4); //speed 19200, Use 8 data bits / even parity / 2 stop bits, compensation 4
  
  NOTES:
  =====
  Not adding compensation with begin(), enables default values in the driver and backward compatibility with
  existing sketches.
    
  Be aware that for different speeds, different compensation might be needed. The lower the speed the 
  less sensitive compensation and thus a higher number.
    
  Be aware that it does not generate a 100% correct speed, but it can be much better.

  If the compensation number is 5 times after each other the same that number will continue to be displayed.

  Often you will see that the compensation is increased by 1 and then decreased by 1 with every try. It can not
  make a 100% match, in the end just pick the compensation number is used most of the time.

  Instead of adding to begin(), you can also change the compensation in the SoftwareSerial.cpp, begin()-routine

  The default compensation is 7 for 9600, 19200 and for 38400.

  The higher the compensation the higher the baudrate.
  
  USAGE:
  =====
  Load this code
  Open Arduino serial monitor at 115200
  Select the BAUDRATE for SoftwareSerial
  The sketch will send every 1 second a byte 0x55
  You do NOT need to add extra wires, no extra connection. It is based on internal measurement.
*/

// define the software serial speed. should not be higher than 38400 for stable connection
#define BAUDRATE 38400

// define PINS. Any pins can be used as long as your board recognises the defined pins
#define RXpin D8    // for Micromod
#define TXpin D7

#include <SoftwareSerial.h>

SoftwareSerial mySerial(RXpin, TXpin); //RX, TX

#define MAXRES 50
uint16_t   res[MAXRES];
uint8_t loopCount = 0;
uint8_t DisplayCount =0;

void setup() {

  Serial.begin(115200);
  Serial.println("Example8: SoftwareSerial TX baudrate compensation");
  Serial.print("Baudrate set at ");
  Serial.println(BAUDRATE);

  // no compensation is provided, thus the default values will be used in the driver
  mySerial.begin(BAUDRATE);

  // enable estimating TX baudrate compensation
  mySerial.estimateTxComp(true);
}

void loop() {
  delay(500); 
  
  // display the results after a number of times
  if (DisplayCount++ > 10) DisplayResult();

  // get and count the current TX baudrate compensation
  res[mySerial.getTxComp()]++;

  if (loopCount++ > 98) {
    Serial.printf("\n================================\n");
    Serial.printf("Test stopped.\n");
    DisplayResult();
    while(1);
  }
    
  mySerial.write(0x55);
}

void DisplayResult()
{
  Serial.printf("\nCompensation results after %d checks:\n",loopCount);
  
  for (uint8_t i = 0; i < MAXRES;i++){
    if (res[i] > 0){
      Serial.printf("Compensation %d, used %d time(s)\n",i, res[i]);
    }
  }
  
  Serial.printf("...........................\n");
  DisplayCount = 0;
}

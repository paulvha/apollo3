/*
 * This sketch is the controller / host for testing IOS (Input/Output Slave) module on an Artemis processor
 * using an Artemis ATP board.
 *
 * This sketch can be on any board, no modifications needed. In my case I was tested with an ESP32 and Artemis
 * micromod and nRF52840 Micromod
 *
 * The controller and peripheral are connected using SPI.
 *
 * The interaction on SPI can either be done with direct access registers or using the FIFO on the IOS.
 * This can be selected with the option below : bool UseFiFo = true;
 *
 * !!!! UseFiFo must be set the same for BOTH peripheral and HOST !!!!
 *
 * A BME280 is connected to the peripheral ATP. This controller has a menu (press <enter> to show) with different options
 * The controller must enable frequent sending of the BME280 results with option 6 of the menu.
 * Other menu-options are self explaning.
 *
 * For more / detailed information see the Apollo3_IOS.odt document with learning.
 *
 * version 1.0 / April 2023 /paulvha
 *
 * peripheral / HOST
  ===============================================================================

     HOST                              ATP peripheral
     --------------------              ----------------
     GPIO handshake                    GPIO[4]  GPIO interrupt (*1)
     IOM0 SPI SCK                      GPIO[0]  IOS SPI SCK
     IOM0 SPI MOSI /COPI               GPIO[1]  IOS SPI MOSI
     IOM0 SPI MISO /CIPO               GPIO[2]  IOS SPI MISO
     IOM0 SPI nCE                      GPIO[3]  IOS SPI nCE
     GND                               GND

    *1) the handshake pin is only used when using FIFO to exchange data

 */

/////////////////////////////////////////////////////////
// TRUE : the peripheral data will be send with FIFO   //
// MUST be set the same for BOTH peripheral and HOST   //
bool UseFiFo = true;

// to receive interrupt from the peripheral with FIFO  //
#define HandShakePin A0

/////////////// Register Map ////////////////////////////
#define CMDREG        0x0     // 8 bits
#define STATREG       0x01
#define HUMIDITY      0x10    // floats (Not used in FIFO mode)
#define PRESSURE      0x14
#define ALTITUDE      0x18
#define TEMPERATURE   0x1C

// bits in Command Reg.
#define CMD_START     0x01  // to be set for regular updates
#define CMD_STOP      0x02
#define CMD_METER     0x04  // request altitude meter
#define CMD_FEET      0x08
#define CMD_CELCIUS   0x10  // request temp in celcius
#define CMD_FAHRNHT   0x20
#define CMD_SEND      0x40  // send data now
#define CMD_DATAREAD  0X80  // data has been read by host

// bits in STATREG
#define STAT_NEWDATA  0X01 // !! NOT used in FIFO mode !!
#define STAT_METER    0X02 // altitude in meter else FEET
#define STAT_CELCIUS  0X04 // temperature in Celsius else Fahrenheit
#define STAT_STARTED  0x08 // auto update enabled
#define STAT_BMEFND   0x10 // set = BME280 FOUND on peripheral

// define the HOST registers (ONLY USED IN FIFO MODE)
#define HOST_IER      0x78 // enable host interrupts
#define HOST_ISR      0x79 // status host interrupts
#define HOST_WCR      0x7A // clear host interrupts
#define HOST_WCS      0x7B // set host interrupts
#define FIFOCTRLO     0x7C // fifoCTR, 8 bit low
#define FIFOCTRUP     0x7D // fifoCTR , 2 bit up
#define FIFO          0x7F // fifo read register

//////////////////////// BME280 /////////////////////////

struct data_to_exchange{
  // BME280 data
  float humidity;
  float pressure;
  float altitude;
  float temperature;
} p;

////////////////////////// SPI ///////////////////////////

#include "SPI.h"

#define CS_PIN A1

#define SPI_SPEED 1000000
#define SPI_ORDER MSBFIRST
#define SPI_MODE SPI_MODE0

SPISettings mySettings(SPI_SPEED, SPI_ORDER, SPI_MODE);

//////////////////////// program  /////////////////////////

// store read status reg (STATREG)
uint8_t statusReg = 0x0;

// store Host status register (HOST_ISR)
uint8_t HostReg = 0x0;

// define serial
#define SERIAL_PORT Serial

volatile bool InterruptReceived = false;

/**
 * commands to send to peripheral
 */
enum key_input{
  REQUEST_METERS = 0x1,
  REQUEST_FEET,
  REQUEST_CELSIUS,
  REQUEST_FRHEIT,
  REQUEST_STOP,
  REQUEST_START,
  REQUEST_NOW
};

/**
 * called when IOS peripheral triggers
 */
//void IRAM_ATTR IosInt() //better for ESP32, but works without as well
void IosInt()
{
  InterruptReceived = true;
}

void setup() {
  SERIAL_PORT.begin(115200);
  do { delay(1000); } while (!SERIAL_PORT);

  SERIAL_PORT.print("Example2 IOS SPI host");
  if (UseFiFo) SERIAL_PORT.println(" using FIFO");
  else SERIAL_PORT.println(" using direct register addressing");

  SPI.begin();

  pinMode(CS_PIN, OUTPUT);

  if (UseFiFo) EnableHandShake();

  ReadStatusReg();

  display_menu();
}

void loop() {

  if (UseFiFo) {
    if (InterruptReceived) {
      handleInterrupt();
    }
  }
  else {
    // check for new data
    ReadStatusReg();

    if (statusReg & STAT_NEWDATA){
      DoBME280Result();
    }
  }

  for(int i = 0; i < 10;i++){
    delay(100);

    // check & handle any keyboard input
    handle_input();
  }
}

/**
 * enable the handshake with the pin and enable interrupts
 */
void EnableHandShake()
{
  // clear all host interrupts in IOS
  if (! WriteReg8(HOST_WCR, 0xff)) {
     SERIAL_PORT.println(F("Something went wrong clearing Host interrupts. Freeze!\r"));
     while(1);
  }

  // enable all host interrupts on IOS
  if (! WriteReg8(HOST_IER, 0xff)) {
     SERIAL_PORT.println(F("Something went wrong setting Host interrupts. Freeze!\r"));
     while(1);
  }

  // enable handshake pin as input
  pinMode(HandShakePin, INPUT_PULLUP);
  attachInterrupt(HandShakePin, IosInt, RISING);
}

/**
 *  check for new data
 */
void handleInterrupt()
{
  InterruptReceived = false;

  if (! ReadReg8(HOST_ISR, &HostReg)) {
    SERIAL_PORT.println("error: could not read host status register");
  }

  if (! WriteReg8(HOST_WCR, HostReg)) {
    SERIAL_PORT.println("error: could not clear interrupt register");
  }

  if (HostReg & 0x1) {
    DoBME280Result();
  }
}

/**
 *  check & handle local keyboard input
 */
void handle_input()
{
  uint8_t input = 0;

  if (! SERIAL_PORT.available()) return;

  delay(100);

  String stringBuffer = SERIAL_PORT.readString();
  char charBuffer[10];

  stringBuffer.toCharArray(charBuffer, 10);
  sscanf(charBuffer, "%d", &input);

  if (input != 0)  handle_cmd(input);

  display_menu();
}

void display_menu()
{
  SERIAL_PORT.println(F("1.  Request altitude in meters\r"));
  SERIAL_PORT.println(F("2.  Request altitude in feet\r"));
  SERIAL_PORT.println(F("3.  Request temperature in Celsius\r"));
  SERIAL_PORT.println(F("4.  Request temperature in Fahrenheit\r"));
  SERIAL_PORT.println(F("5.  Request STOP sending data\r"));
  SERIAL_PORT.println(F("6.  Request (re)START sending data\r"));
  SERIAL_PORT.println(F("7.  Request NOW sending data\r"));
}

/**
 * Enter and send to peripheral
 */
void handle_cmd(uint8_t req)
{
  uint8_t Command = 0x0;

  switch (req)
  {
    case REQUEST_METERS:
      SERIAL_PORT.print(F("Request in meters\r\n"));
      Command |= CMD_METER;
      break;

    case REQUEST_FEET:
      SERIAL_PORT.print(F("Request in feet\r\n"));
      Command |= CMD_FEET;
      break;

    case REQUEST_CELSIUS:
      SERIAL_PORT.print(F("Request in Celsius\r\n"));
      Command |= CMD_CELCIUS;
      break;

    case REQUEST_FRHEIT:
      SERIAL_PORT.print(F("Request in Fahrenheit\r\n"));
      Command |= CMD_FAHRNHT;
      break;

    case REQUEST_STOP:
      SERIAL_PORT.print(F("Request Stop sending\r\n"));
      Command |= CMD_STOP;
      break;

    case REQUEST_START:
      SERIAL_PORT.print(F("Request (re)Start sending\r\n"));
      Command |= CMD_START;
      break;

    case REQUEST_NOW:
      SERIAL_PORT.print(F("Request Sending BME280 info now\r\n"));
      Command |= CMD_SEND;
      break;

    default:
      SERIAL_PORT.print("Unknown request: ");
      SERIAL_PORT.println(req);
      break;
  }

  if (Command) {
    // update command register /  write to peripheral
    if (! WriteReg8(CMDREG, Command)) {
      SERIAL_PORT.println("error during sending command");
    }
  }
}

/**
 * read status register
 */
void ReadStatusReg()
{
  if (! ReadReg8(STATREG, &statusReg)) {
    SERIAL_PORT.println("error: could not read status register");
  }
}

/**
 *  read and display the BME280 results
 */
void DoBME280Result()
{
  uint8_t buf[16];
  uint16_t fifoLength = 16;
  uint8_t v;

  if (UseFiFo) {

    if (! ReadReg8(FIFOCTRLO, &v)){
      SERIAL_PORT.println("error during reading FIFOCTRLO");
    }

    fifoLength = v;

    if (! ReadReg8(FIFOCTRUP, &v)){
      SERIAL_PORT.println("error during reading FIFOCTRUP");
    }
    fifoLength |= v << 8;

    if (fifoLength != 16) {
      SERIAL_PORT.print("WARNING : Fifolength is ");
      SERIAL_PORT.print(fifoLength);
      SERIAL_PORT.println(" NOT the expected 16");
    }

    SPI.beginTransaction(mySettings);
    // take the chip select low to select the device:
    digitalWrite(CS_PIN, LOW);

    // send the device the FIFO register to read:
    SPI.transfer(FIFO & 0x7f);

  }
  else {  // direct register access

    SPI.beginTransaction(mySettings);

    // take the chip select low to select the device:
    digitalWrite(CS_PIN, LOW);

    // send the device the register you want to read:
    SPI.transfer(HUMIDITY & 0x7f);
  }

  // if too many bytes in the fifo, read the oldest away
  if (fifoLength > 16){
    SERIAL_PORT.println("Trying removing oldest");
    do {
      SPI.transfer(0x00);
    } while (--fifoLength != 16 );
  }

  uint8_t i = 0;

  while ( i < fifoLength ) {
    buf[i] = SPI.transfer(0x00); // receive a bytes
    i++;
  }

  // take the chip select high to de-select:
  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();

  // get the latest status setting
  ReadStatusReg();

  struct data_to_exchange *p = (data_to_exchange *) buf;

  // temperature
  SERIAL_PORT.print(F("\r\nTemperature\t"));
  SERIAL_PORT.print(p->temperature);
  if (statusReg & STAT_CELCIUS)  SERIAL_PORT.println("*C\r");
  else  SERIAL_PORT.println(F("*F\r"));

  // humidity
  SERIAL_PORT.print(F("Humidity\t"));
  SERIAL_PORT.print(p->humidity);
  SERIAL_PORT.println("%\r");

   // pressure
  SERIAL_PORT.print(F("Pressure\t"));
  SERIAL_PORT.print(p->pressure);
  SERIAL_PORT.println(F(" hPa\r"));

  // altitude
  SERIAL_PORT.print(F("Altitude\t"));
  SERIAL_PORT.print(p->altitude);
  if (statusReg & STAT_METER) SERIAL_PORT.println(F(" Meter\r\n"));
  else SERIAL_PORT.println(F(" Feet\r\n"));

  // let peripheral know it has been read
  if (! WriteReg8(CMDREG, CMD_DATAREAD)) {
      SERIAL_PORT.println("error during sending command");
  }
}

/**
 * read a register from peripheral
 */
bool ReadReg8(uint8_t reg, uint8_t *data)
{
  SPI.beginTransaction(mySettings);

  // take the chip select low to select the device:
  digitalWrite(CS_PIN, LOW);

  // send the device the register you want to read:
  SPI.transfer(reg & 0x7f);

  // receive the register content
  *data = SPI.transfer(0x00); // receive a byte as character

  // take the chip select high to de-select:
  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();
  delay(100);

  return true;
}

/**
 * Write to an 8 bit register on the peripheral
 */
bool WriteReg8(uint8_t reg, uint8_t data)
{
  SPI.beginTransaction(mySettings);

  // take the chip select low to select the device:
  digitalWrite(CS_PIN, LOW);

  // send the device the register you want to write:
  SPI.transfer(reg | 0x80);   //Ored with "write request" bit

  // send a value
  SPI.transfer(data);

  // take the chip select high to de-select:
  digitalWrite(CS_PIN, HIGH);

  SPI.endTransaction();
  delay(100);

  return true;
}

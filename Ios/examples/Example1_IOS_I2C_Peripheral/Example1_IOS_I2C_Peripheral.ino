/*
 * 
 * This sketch is the peripheral for testing the IOS (Input/Output Slave) module on an Artemis processor
 * using an Artemis ATP board.
 * 
 * A BME280 is connected with SPI to the ATP. The ATP is connected with I2C to a controller. 
 * 
 * This has been tested with Apollo3 library version V1.2.3. It requires a adding  
 * a new library IOS.cpp / IOS.h and new IOSlave.cpp / IOSlave.h. 
 * 
 * The Controller / host can be on any board, no modifications needed. In my case I was tested 
 * with an ESP32 and Artemis micromod and nRF52840 Micromod 

 * The controller and peripheral are connected using I2C.
 * 
 * The interaction on I2C can either be done with direct access registers or using the FIFO on the IOS. 
 * This can be selected with the option below : bool UseFiFo = true;
 * !!!! UseFiFo must be set the same for BOTH peripheral and HOST !!!!
 * 
 * A BME280 is connected to the peripheral ATP with SPI. There is a menu (press <enter> to show) 
 * with different options that can be used by BOTH peripheral and controller. 
 * For requent sending of the BME280 results set option 6 of the menu. 
 * Other menu-options are self explaning.
 * 
 * For more / detailed information see the Apollo3_IOS.odt document with learning. 
 * 
 * version 1.0 / April 2023 /paulvha
 * 
 *   BME280
  ===============================================================================
  This has been tested with an Adafruit BME280 connected to an Artemis ATP board

    ATP         BME280
                VIN 
    3V3         3v3
    GND         GND
    SCK         SCK
    MISO        SDO  
    MOSI        SDI  
    CS_PIN      CS

    CS_PIN for ATP is defined below.
    
  *  peripheral / HOST communicating on SPI
  ===============================================================================    
    Peripheral  controller / host  
    ATP         Other Board
     0          SCL  --pullup--!-- +3V3
     1          SDA  --pullup--!
     4          Interrupt_pin (*1)
    GND         GND

    *1) the interrupt pin is only necessary when using FIFO to exchange data
 */

#include "SparkFunBME280.h"
#include "Ios.h"

/////////////////////////////////////////////////////////
// true : the peripheral data will be send with FIFO   //
// MUST be set the same for BOTH peripheral and HOST   //
bool UseFiFo = true;

/////////////// BME280 /////////////////////////

// sent temperature in Celsius (1) or Fahrenheit (0)
bool TempInCelsius = true;

// sent altitude in meters (1) or feet (0)
bool AltitudeInMeter = true;

// sets local pressure on sealevel to calculate altitude correctly
#define PressureSeaLevel   101325.0

// FIRST interval in seconds after connect
#define FIRST_SENDINTERVAL  10

// after interval in seconds between sending an update
#define SENDINTERVAL  30

// BME280 detected
bool BME280_Detected = false;

// BME280 chip select
#define CS_PIN 28

struct data_to_exchange{
  // BME280 data
  float humidity;
  float pressure;
  float altitude;
  float temperature;
} p;

/////////////// Register Map /////////////////////////

#define CMDREG        0x0     // 8 bits
#define STATREG       0x01

#define HUMIDITY      0x10    // 32 bit / floats 
#define PRESSURE      0x14
#define ALTITUDE      0x18
#define TEMPERATURE   0x1C

// bits in Command Reg.
#define CMD_START     0x01  // to be set by host
#define CMD_STOP      0x02
#define CMD_METER     0x04  // request altitude meter
#define CMD_FEET      0x08
#define CMD_CELCIUS   0x10  // request temp in celcius
#define CMD_FAHRNHT   0x20
#define CMD_SEND      0x40  // send data now
#define CMD_DATAREAD  0X80  // data has been read by host

// bits in STATREG
#define STAT_NEWDATA   0X01 // !! Not used when sending FIFO !!
#define STAT_METER     0X02 // altitude in meter else FEET
#define STAT_CELCIUS   0X04 // temperature in Celsius else Fahrenheit
#define STAT_STARTED   0x08 // auto update enabled
#define STAT_BMEFND    0x10 // set = BME280 FOUND

/////////////// I2C ////////////////////////////

const uint8_t IOS_address = 0x42;

/////////////// program  /////////////////////////

// define serial
#define SERIAL_PORT Serial

// enable sketch data (disable : comment out)
#define SKETCH_SHOW_DATA

uint16_t Interval = FIRST_SENDINTERVAL;  // send first data x seconds after connect
bool RequestSendingNow = false;          // option 7 menu
bool EnableSend = false;                 // will start sending regular update
bool NewData = false;                    // new update available and not read by host

/*
 * Act on command. either received from the local serial or received from the connected central
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

/////////////// constructors /////////////////////
BME280 mySensor;

void setup() {
  SERIAL_PORT.begin(115200);
  do { delay(1000); }while (!SERIAL_PORT);
  
  SERIAL_PORT.print("Example1 IOS peripheral with I2C");
  if (UseFiFo) SERIAL_PORT.println(" using FIFO");
  else SERIAL_PORT.println(" using direct register addressing");
  
  // >>>>>>>>> init BME280 <<<<<<<<<<<<<<<<<<
  
  if (mySensor.beginSPI(CS_PIN) == false) // Begin communication over SPI
  {
    SERIAL_PORT.println(F("The BME280 did not respond. Please check wiring. freeze! \r"));
    while(1);
  }

  SERIAL_PORT.println(F("The BME280 detected.\r"));
  BME280_Detected = true;
  
  // sets local pressure on sealevel to calculate altitude correctly
  mySensor.setReferencePressure(PressureSeaLevel);

  // >>>>>>>>> init IOS <<<<<<<<<<<<<<<<<<

  // start IOS in I2C mode
  IOS.begin(IOS_address, UseFiFo);

  // set call back
  IOS.onReceive(HostReceive);

  UpdateStatusReg();

  display_menu();
}

void loop() {
  
  // A pending request from host
  if (RequestSendingNow){
    RequestSendingNow = false;
    handle_cmd(REQUEST_NOW);
  }

  // if allowed to send regular BME280 update.
  if (EnableSend) {

    // Interval for reading & sending BME280
    if (--Interval == 0 ){
      CreateUpdate();
      Interval = SENDINTERVAL;
    }
  }

  // wait 10 x 100mS while checking
  for (byte i = 0; i < 10; i++){
    if (RequestSendingNow) break;
      
    // handle any keyboard input
    handle_input();
    delay(100);
  }
}

/** 
 *  handle local keyboard input
 */
void handle_input()
{
  uint8_t input=0;

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

// could be entered on peripheral or host command register
void handle_cmd(uint8_t req)
{
  switch (req)
  {
    case REQUEST_METERS:
      SERIAL_PORT.print(F("Report in meters\r\n"));
      AltitudeInMeter = true;
      break;

    case REQUEST_FEET:
      SERIAL_PORT.print(F("Report in feet\r\n"));
      AltitudeInMeter = false;
      break;

    case REQUEST_CELSIUS:
      SERIAL_PORT.print(F("Report in Celsius\r\n"));
      TempInCelsius = true;
      break;

    case REQUEST_FRHEIT:
      SERIAL_PORT.print(F("Report in Fahrenheit\r\n"));
      TempInCelsius = false;
      break;

    case REQUEST_STOP:
      SERIAL_PORT.print(F("Stop sending\r\n"));
      EnableSend = false;
      break;

    case REQUEST_START:
      SERIAL_PORT.print(F("(re)Start sending\r\n"));
      EnableSend = true;
      Interval = 1;     // send at next turn
      break;

    case REQUEST_NOW:
      SERIAL_PORT.print(F("Sending BME280 info now\r\n"));
      CreateUpdate();
      Interval = SENDINTERVAL; // next one after SENDINTERVAL if enabled
      break;

    default:
      SERIAL_PORT.printf("Unknown request, 0x%x\r\n", req);
      break;
  }

  // update status register
  UpdateStatusReg();
}

/**
 * update the status register
 */
void UpdateStatusReg()
{
  // in case of FIFO let interrupt the host if new data
  if (UseFiFo && NewData) {

    if ( ! IOS.set_interrupt_host(AM_HAL_IOS_IOINTCTL_INT0)) {
     SERIAL_PORT.printf("Error during sending interrupt host\n");
    }
  }
  
  uint8_t statusReg = 0x0;
  
  if (BME280_Detected) statusReg |= STAT_BMEFND;
  if (AltitudeInMeter) statusReg |= STAT_METER;
  if (TempInCelsius)   statusReg |= STAT_CELCIUS;
  if (EnableSend)      statusReg |= STAT_STARTED;
  if (NewData)         statusReg |= STAT_NEWDATA;
  
  // write to host
  uint8_t ToSend[2];

  ToSend[0] = STATREG;
  ToSend[1] = statusReg;
  
#ifdef SKETCH_SHOW_DATA
 //SERIAL_PORT.printf("statusReg %x\n", statusReg);
#endif

  // write to update the status REGISTER (false) NOT the FIFO
  uint8_t ret = IOS.write(ToSend, 2,false);
}

/**
 * When receiving data bytes from Controller, this function is called as an interrupt
 */
void HostReceive(int bytesReceived) {

  uint8_t Register = IOS.read();

  //SERIAL_PORT.printf("register %x\n", Register);
  if (Register != CMDREG)  return;

  uint8_t Command = IOS.read();

  if (Command & CMD_METER)   handle_cmd(REQUEST_METERS);
  if (Command & CMD_FEET)    handle_cmd(REQUEST_FEET);
  if (Command & CMD_CELCIUS) handle_cmd(REQUEST_CELSIUS);
  if (Command & CMD_FAHRNHT) handle_cmd(REQUEST_FRHEIT);
  if (Command & CMD_STOP)    handle_cmd(REQUEST_STOP);
  if (Command & CMD_START)   handle_cmd(REQUEST_START);
  if (Command & CMD_SEND)    RequestSendingNow = true;
  
  if (Command & CMD_DATAREAD){
    NewData = false;
    UpdateStatusReg();
  }

  if (bytesReceived > 1) {
    SERIAL_PORT.printf("Not expecting more 1 byte  %d\n", bytesReceived);
  }
}

/**
 * Read the BME280 values into the structure and send
 */
void CreateUpdate()
{
  if (! BME280_Detected) {
    SERIAL_PORT.println(F("No BME280 Detected\r"));
    memset(&p,0x0,sizeof(struct data_to_exchange));
    UpdateHost((uint8_t *) &p, sizeof(struct data_to_exchange));
    return;
  }

  if (TempInCelsius) p.temperature = mySensor.readTempC();
  else p.temperature = mySensor.readTempF();

  p.humidity  = mySensor.readFloatHumidity();
  p.pressure = mySensor.readFloatPressure() / 100;

  if (AltitudeInMeter) p.altitude = mySensor.readFloatAltitudeMeters();
  else p.altitude = mySensor.readFloatAltitudeFeet();

#ifdef SKETCH_SHOW_DATA

  SERIAL_PORT.print(F("\r\nTemperature\t"));
  SERIAL_PORT.print(p.temperature);
  if (TempInCelsius) SERIAL_PORT.println("*C\r");
  else SERIAL_PORT.println(F("*F\r"));

  SERIAL_PORT.print(F("Humidity\t"));
  SERIAL_PORT.print(p.humidity);
  SERIAL_PORT.println(F("%\r"));

  SERIAL_PORT.print(F("Pressure\t"));
  SERIAL_PORT.print(p.pressure);
  SERIAL_PORT.println(F(" hPa\r"));

  SERIAL_PORT.print(F("Altitude\t"));
  SERIAL_PORT.print(p.altitude);
  if (AltitudeInMeter) SERIAL_PORT.println(F(" Meter\r"));
  else SERIAL_PORT.println(F(" Feet\r"));

#endif

  UpdateHost((uint8_t *) &p, sizeof(struct data_to_exchange));
}

/**
 * sent a data message to the controller / host
 *
 * Format data :
 * buf =  data to be sent
 * len  = length of data
 * 
 * instead of writing each register, we write the 16 bits directly
 * This also enable that FIFO is selected in the library
 *
 */
bool UpdateHost(uint8_t *buf, uint8_t len)
{
  int i, j = 0 ;             // conversion counter
  uint8_t ToSend[17];
    
  ToSend[j++] = HUMIDITY;   // set starting register (ignored in FIFO mode)

  // copy data
  for (i = 0 ; i < len ; i++) ToSend[j++] = buf[i];

  uint8_t ret = IOS.write(ToSend, j, UseFiFo);

  if (ret != j) {
    SERIAL_PORT.printf("Something went wrong ret %d, j %d\n", ret,j);
    return false;
  }
  
  // update status register
  NewData = true;
  UpdateStatusReg();

  return(true);
}

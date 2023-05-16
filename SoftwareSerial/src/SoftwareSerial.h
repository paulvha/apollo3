/*
  This is a library written for the SparkFun Artemis

  *********************************************************************
  * This is a special port of Software Serial for Library version 2.0.6
  * While there is an option to increase the speed above 9600, given the
  * long time it takes the V2 library to trigger the interrupt handler
  * of Software Serial receiving speed to max 9600 is supported.
  * For sending only upto 57600 can be achieved.
  * See the readme in the library / paulvha
  *********************************************************************

  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support open source hardware. Buy a board!
  https://www.sparkfun.com/artemis

  Written by Nathan Seidle @ SparkFun Electronics, August 12th, 2019

  https://github.com/sparkfun/SparkFun_Apollo3

  SoftwareSerial support for the Artemis
  Any pin can be used for software serial receive or transmit
  at 300 to 115200bps and anywhere inbetween.
  Limitations (similar to Arduino core SoftwareSerial):
    * RX on one pin at a time.
    * No TX and RX at the same time.
    * TX gets priority. So if Artemis is receiving a string of characters
    and you do a Serial.print() the print will begin immediately and any additional
    RX characters will be lost.
    * Uses Timer/Compare module H (aka 7). This will remove PWM capabilities
    on pads 11, 37, 48, and 49.
    * Parity is supported during TX but not checked during RX.
    * Enabling multiple ports causes 115200 RX to fail (because there is additional instance switching overhead)

  Development environment specifics:
  Arduino IDE 1.8.x

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SoftwareSerial_H
#define _SoftwareSerial_H
#include "Arduino.h"
#include <Stream.h>
#include "gpio_irq_api.h"

#define AP3_SS_BUFFER_SIZE 128 //Limit to 128 bytes

#define TIMER_FREQ 3000000L

/** Added October 2021
    By default MBED is bypassed for handling the RX interrupt.
    This saves execution time and allows an RX speed upto 38400 baud else
    the maximum speed is 19200

    Using MBED the timedelay between a level change on the line and detection
    in SoftwareSerial is ~22us. Bypassing MBED the timedelay is ~18us.

    The interrupt handler itself takes 3.3us if the level change happend after
    1 bit time, 4.6us after 2 bit times, 6.9us after 4 bit times, 7.8us after 5 bits.
    so about 1.2us for an extra bit.

    On 38400 baud the bit time is 26us. With bypassing we can handle the
    interrupt within a single bit time. Without bypassing we just on the
    boundery and run higher risk on missing the next bit.

    So extended explaned.odt which can be read with any word processor

    By defining bypass below, the interrupt is directly routed
    to Software Serial. */

#define BYPASS_MBED_INTERRUPT 1

class SoftwareSerial : public Stream
{
public:
  SoftwareSerial(PinName rxPin, PinName txPin, bool invertLogic = false);
  ~SoftwareSerial();

  void listen();
  void stopListening();
  bool isListening();

  void begin(uint32_t baudRate, int comp = 0);
  void begin(uint32_t baudRate, uint16_t SSconfig, int comp = 0);
  void end(void);

  int available();
  int read();
  int peek();
  void flush();
  bool overflow();

  virtual size_t write(uint8_t toSend);
  virtual size_t write(const uint8_t *buffer, size_t size);
  virtual size_t write(const char *str);

  void txHandler(void);
  void estimateTxComp(bool act);    // may 2023
  int16_t getTxComp();              // may 2023

  void rxBit(void);
  void rxEndOfByte(void);

  volatile bool rxInUse = false;
  volatile bool txInUse = false;

private:
  ap3_err_t softwareserialSetConfig(uint16_t SSconfig);
  void beginTX();

  uint8_t txBuffer[AP3_SS_BUFFER_SIZE];
  uint16_t txBufferHead = 0;
  volatile uint16_t txBufferTail = 0;
  volatile uint8_t outgoingByte = 0;

  volatile uint8_t rxBuffer[AP3_SS_BUFFER_SIZE];
  volatile uint8_t rxBufferHead = 0;
  uint8_t rxBufferTail = 0;
  volatile uint8_t incomingByte = 0;

  PinName _rxPin;
  PinName _txPin;

  uint8_t _dataBits = 0; //5, 6, 7, or 8
  uint8_t _parity = 0;   //Type of parity (0, 1, 2)
  uint8_t _stopBits = 0;
  uint8_t _parityBits = 0; //Number of parity bits (0 or 1)
  uint8_t _ExpectBits = 0;
  bool _invertLogic;

  //For RX
  uint16_t rxSysTicksPerBit = 0;
  uint32_t rxSysTicksPerByte = 0;

  volatile uint8_t numberOfBits[10];
  volatile uint32_t lastBitTime = 0;
  volatile uint8_t bitCounter;
  volatile bool bitType = false;
  bool _rxBufferOverflow = false;

#ifdef BYPASS_MBED_INTERRUPT
  gpio_t gpio;                      // handle for GPIO
  gpio_irq_t gpio_irq;              // handle interrupt outside Mbed

  void SetInterrupt(PinName pinName);
  void RemoveInterrupt(PinName pinName);
#endif

  // get TX baudrate estimation May 2023
  bool _BaudrateCompensation = false;
  unsigned long _Txst;
  unsigned long _Txtime;
  void HandleEstimateComp();

  // For TX
  void calcParityBit();
  uint16_t txSysTicksPerBit = 0;
  uint16_t txSysTicksPerStopBit = 0;
  uint8_t _parityForByte = 0; //Calculated per byte
  bool _txBufferOverflow = false;
  uint32_t _baudRate;
  int16_t _comp;
  void calcTicks();
};

#endif

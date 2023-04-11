/*
Copyright (c) 2019 SparkFun Electronics
Copyright (c) 2023 SFs

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _AP3_IOS_H_
#define _AP3_IOS_H_

#include "Arduino.h"
#include "ioslave/ap3_ioslave.h"
#include "ioslave/ap3_handler.h"

// FIFO
// LRAM + SRAM is max 1023.
// LRAM is assumed 128.. thus setting SRAM to 895
#define AM_IOS_TX_BUFSIZE_MAX   895

#define AP3_IOS_RX_BUFFER_LEN 128

class AP3_IOS : public IOSlave
{
public:
  AP3_IOS(uint8_t iom_instance);

  /**
   * begin communicating peripheral / host SPI mode
   * @param EnableFiFo : true => use FIFO, false => use direct register
   */
  void beginSPI(bool EnableFifo = false);

  /**
   * begin communicating peripheral / host I2C mode
   * @param EnableFiFo : true => use FIFO, false => use direct register
   * @param enableGeneralCall " true -> detect host sending on I2Caddress 0
   */
  void begin(uint8_t address, bool EnableFifo = false, bool enableGeneralCall = false);

  /**
   * power down and reset pins
   */
  void end();

  /**
   * @param data : data to be written
   * @param quantity : total bytes in data
   * @param UseFifo :
   * 	true  : data will be send to fifo
   *  false : write values(s) to the starting register
   *
   * data[0] = starting register
   * data[1 ...] data be written
   * quantity : total # entries in data (include register)
   *
   * return quantity used (including register)
   */
  size_t write(uint8_t *data, size_t quantity, bool UseFifo);

  int available(void);
  int read(void);
  int peek(void);
  void flush(void);

  void onReceive(void (*)(int));

  /**
   * in FIFO mode, use the HOST registers to raise an interrupt on
   * on the interrupt pin
   *
   * #define AM_HAL_IOS_IOINTCTL_INT0    (0x01)
   * #define AM_HAL_IOS_IOINTCTL_INT1    (0x02)
   * #define AM_HAL_IOS_IOINTCTL_INT2    (0x04)
   * #define AM_HAL_IOS_IOINTCTL_INT3    (0x08)
   * #define AM_HAL_IOS_IOINTCTL_INT4    (0x10)
   * #define AM_HAL_IOS_IOINTCTL_INT5    (0x20)
   *
   * return :
   * True if OK
   * False if interrupt was not enabled by the HOST
   */
  bool set_interrupt_host(uint32_t mask);

  /**
   * call back from ap3_ioslave.cpp
   */
  void onService();

private:
  uint8_t _ios_instance;

  RingBufferN<AP3_IOS_RX_BUFFER_LEN> _rxBuffer;     // RX Buffer

  am_hal_ios_config_t _PeConfig;                    // IOS / peripheral configuration
  uint8_t TxFifoBuffer[AM_IOS_TX_BUFSIZE_MAX];

  // Callback user functions
  void (*_onReceiveCallback)(int);
};

extern AP3_IOS IOS;

#endif // _AP3_IOS_H_

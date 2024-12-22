/*
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

// IOSlave is a parent class that contains SPI and I2C functionality for the Apollo3

#ifndef _AP3_IOSLAVE_H_
#define _AP3_IOSLAVE_H_

#include "Arduino.h"
#include "ap3_handler.h"

#define AM_HAL_IOS_INT_ERR  (AM_HAL_IOS_INT_FOVFL | AM_HAL_IOS_INT_FUNDFL | AM_HAL_IOS_INT_FRDERR)
#define AM_HAL_IOS_XCMP_INT (AM_HAL_IOS_INT_XCMPWR | AM_HAL_IOS_INT_XCMPWF | AM_HAL_IOS_INT_XCMPRR | AM_HAL_IOS_INT_XCMPRF)

/**
 * There are 2 ways to capture the direct access register that has been written.
 * use an interrupt based on REGACCEN (so when a register has been written)
 * use an interrupt based on COMPWR (so when an IO write is completed)
 *
 * Do not use BOTH !!
 */

class IOSlave {
private:

protected:
  am_hal_ios_config_t _ios_config;  // holds the IOS config
  uint8_t _ios_instance;            // holds IOS isntance (always 0)
  bool    _EnableFifo;              // True : use FIFO to send to host
  int RecStartReg = 0xFF;           // holds the starting register host written by host
  int RecByteCnt = 0;               // holds number of bytes written
  uint8_t _GenData;                 // holds data written when address 0x0 was used
  bool    _enableGeneralCall;       // enable d detection address 0x0
  void (*_onWriteCallback)(int);    // holds additonal call back

public:
  IOSlave(uint8_t ios_instance);
  /**
   * initialize
   * @param config           : set IOS configuration
   * @param EnableFifo       : write to fifo
   * @param enableGeneralCall: detect host writing to address zero (I2C only)
   */
   ap3_err_t ios_initialize(am_hal_ios_config_t config, bool EnableFifo, bool enableGeneralCall);


  /**
   * deinitialize IOS and power down
   */
   ap3_err_t ios_deinitialize( void );

   void ios_acc_isr(void);     // interrupt routine called from "c"
   void ios_host_isr(void);

  /**
   * call this to enable an interrupt when host has written to any
   * of the host registers
   */
   ap3_err_t ios_receiveHostInterrupt();

  /**
   * add extra call back from sketch
   * bypass Ios.cpp
   */
   void ios_onWrite(void (*function)(int));

  /**
   * set which direct Register(s) to trigger interrupt
   * By default ALL registers are enabled.
   * @param reg : set bit to enable/disable registers
   *
   * bit 31 (the MSB) is reg o
   * bit 30 is reg 1
   * etc.
   *
   * First do ios_getRegAccEnable() modify and write back
   */
   void ios_setRegAccEnable(uint32_t reg);

  /**
   * Get current Register Direct access interrupt enabled
   */
   uint32_t ios_getRegAccEnable();

  /**
   * return number of bytes received in last host write
   */
   int ios_available();

  /**
   * return starting register in last host write
   */
   int ios_get_register();

  /**
   * Write register(s) in the direct Access register space
   * @param reg  : register between 0x0 - 0x4F
   * @param data : pointer to the data to write
   * @param len  : length of data to write
   *
   * return:
   *   number of bytes written
   */
   size_t ios_write_reg(uint8_t reg, uint8_t *data, uint8_t len);

  /**
   * write to fifo
   * @param data : pointer to the data to write
   * @param len  : length of data to write
   *
   * return
   * number of bytes written
   */
   size_t ios_write_fifo(uint8_t *data, uint8_t len);

  /**
   * set host interrupt register. Mask selection can be
   *
   * #define AM_HAL_IOS_IOINTCTL_INT0    (0x01)
   * #define AM_HAL_IOS_IOINTCTL_INT1    (0x02)
   * #define AM_HAL_IOS_IOINTCTL_INT2    (0x04)
   * #define AM_HAL_IOS_IOINTCTL_INT3    (0x08)
   * #define AM_HAL_IOS_IOINTCTL_INT4    (0x10)
   * #define AM_HAL_IOS_IOINTCTL_INT5    (0x20)
   *
   * Return :
   * true is interrupt was enabled by host
   * false if not enabled
   */
   bool ios_interrupt_host(uint32_t mask);

  /**
   * Read register(s) in the direct Access register space
   * @param reg  : register between 0x0 - 0x4F
   * @param data : pointer to store the data read
   * @param len  : length of data to read
   *
   * return
   *   Number of bytes read
   */
   size_t ios_read_reg(uint8_t reg, uint8_t *data, uint8_t len);

  /**
   * Read a byte from a register
   * @param reg : register to read
   *
   * return :
   *  register content or 0xff if out of range
   */
   uint8_t ios_read_single_reg(uint8_t reg);
};

#endif // _AP3_IOSLAVE_H_

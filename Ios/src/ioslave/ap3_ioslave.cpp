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

// enable debug messages
//#define IOS_DEBUG

#include "ap3_ioslave.h"

void* _ios_handle;

// used to connect between C-interrupt and CPP interrupt routine
IOSlave *ap3_IOS_handle = 0;

// pad 4 interrupt pin setting
const am_hal_gpio_pincfg_t g_AM_BSP_GPIO_SLINT =
{
  .uFuncSel            = AM_HAL_PIN_4_SLINT,
  .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
  .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
  .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVEHIGH,
};

/**
 * create constructor
 */
IOSlave::IOSlave(uint8_t ios_instance)
{
  _ios_instance = ios_instance;
  _EnableFifo = false;
  _enableGeneralCall = false;
  _ios_handle = NULL;
}

/**
 *  called when when a setting in IOINTEN triggers
 */
inline void IOSlave::ios_host_isr() {

  uint32_t  ui32IOSTAT;

  //
  // Check to see what caused this interrupt, then clear the bit from the
  // interrupt register.
  //
  am_hal_ios_interrupt_status_get(_ios_handle, false, &ui32IOSTAT);
  am_hal_ios_interrupt_clear(_ios_handle, ui32IOSTAT);

#ifdef IOS_DEBUG
  Serial.printf("ios_host_isr %lx\n", ui32IOSTAT);
  if (ui32IOSTAT & AM_HAL_IOS_INT_XCMPWF) Serial.println("\tFifo has been written by host oh..NO!!");
  if (ui32IOSTAT & AM_HAL_IOS_INT_XCMPRF) Serial.println("\tFifo has been read by host");
  if (ui32IOSTAT & AM_HAL_IOS_INT_IOINTW) Serial.println("\tHost register has been written by host");

  // others handled below
#endif

  if (ui32IOSTAT & AM_HAL_IOS_INT_FUNDFL) // only with FIFO
  {
#ifdef IOS_DEBUG
    Serial.printf("\tHitting underflow for the requested IOS FIFO transfer\n");
#endif
    // We should never hit this case unless the threshold has beeen set
    // incorrect, or we are unable to handle the data rate
    // ERROR!
    am_hal_debug_assert_msg(0,
        "Hitting underflow for the requested IOS FIFO transfer.");
  }

  if (ui32IOSTAT & AM_HAL_IOS_INT_ERR)
  {
#ifdef IOS_DEBUG
    Serial.printf("\tHitting ERROR case\n");
#endif
    // We should never hit this case
    // ERROR!
    am_hal_debug_assert_msg(0,
        "Hitting ERROR case.");
  }

  if (ui32IOSTAT & AM_HAL_IOS_INT_FSIZE) // only with FIFO
  {
#ifdef IOS_DEBUG
    Serial.printf("\tFSIZE Alert\n");
#endif
    //
    // Service the IOS FIFO if necessary.
    //
    am_hal_ios_interrupt_service(_ios_handle, ui32IOSTAT);
  }

  // if something has been written to I2C address zero
  if (ui32IOSTAT & AM_HAL_IOS_INT_GENAD)
  {

#ifdef IOS_DEBUG
    Serial.printf("\tGeneral addressing\n");
#endif

    RecByteCnt = 1;
    RecStartReg = 0x80;     // means general data

    // get the data byte that was send
    _GenData = IOSLAVEn(_ios_instance)->GENADD_b.GADATA;

    // Now do something with that data byte
    am_ios_handler();
  }

  // read complete (something has been read)
  if (ui32IOSTAT & AM_HAL_IOS_INT_XCMPRR)
  {

#ifdef IOS_DEBUG
    Serial.printf("\tXCMPRR : A read completed\n");
#endif
  }

  // write complete (something received)
  if (ui32IOSTAT & AM_HAL_IOS_INT_XCMPWR)
  {
    // obtain the Direct Access Status and reset
    uint32_t AccIntStat = IOSLAVEn(_ios_instance)->REGACCINTSTAT;
    IOSLAVEn(_ios_instance)->REGACCINTCLR = AccIntStat;
#ifdef IOS_DEBUG
    Serial.printf("\tXCMPWR Statreg %lx\n",AccIntStat);
#endif
    RecStartReg = 0xFF;
    RecByteCnt = 0;

    // parse the Direct access area
    for (int8_t i = 31 ; i != -1 ; i--) {

      if ( AccIntStat & (1 << i) ) {
        // save starting register
        if (RecStartReg == 0xff) {
          RecStartReg = 31 - i;
        }
        RecByteCnt++;
      }
    }

    /* in AccIntStat
     * bit 31 - 16 represent single byte registers 0x00 - 0xF
     * bit 15 - 0  represent 4 byte registers 0x10 - 0x4f
     * hence the ByteCnt is times 4 in that later area
     * You MUST write on a 4 byte boundery !!
     */
    if (RecStartReg > 0xF) RecByteCnt = RecByteCnt * 4;

#ifdef IOS_DEBUG
    Serial.printf("\tStarting register 0x%lx, number of bytes written %d\n", RecStartReg, RecByteCnt);

    uint8_t *pui8Packet = (uint8_t *) am_hal_ios_pui8LRAM;
    for (uint8_t j = 0; j < RecByteCnt; j++) {
      Serial.printf("\tregister: 0x%x, value: 0x%x\n", RecStartReg+j, pui8Packet[RecStartReg+j]);
    }
#endif
    // call back to Ios.CPP -> onService
    am_ios_handler();

    // in case an extra call back was subscribed
    if (_onWriteCallback)
    {
      _onWriteCallback(RecStartReg);
 		}
  }
}

/**
 * set  which Registers to trigger interrupt
 * @param reg : set bit to enable registers
 *
 * bit 31 (the MSB) is reg o
 * bit 30 is reg 1
 * etc.
 *
 */
void IOSlave::ios_setRegAccEnable(uint32_t reg)
{
  NVIC_DisableIRQ(IOSLAVEACC_IRQn);

  IOSLAVEn(_ios_instance)->REGACCINTCLR = AM_HAL_IOS_ACCESS_INT_ALL;
  IOSLAVEn(_ios_instance)->REGACCINTEN = reg;

  NVIC_EnableIRQ(IOSLAVEACC_IRQn);
}

/**
 * Get current Register Direct access interrupt enable
 */
uint32_t IOSlave::ios_getRegAccEnable()
{
  return(IOSLAVEn(_ios_instance)->REGACCINTEN);
}

/**
 * This is called when something has been written to any of the IOS direct addresses
 * THIS ONLY WORKS GOOD if register 0x00 - 0x0F are used as 8 bit registers and 0x10 - 0x4C are used as either 16 or 32 bits registers
 * For usage as 16 bit, start writting with 2 bytes offset e.g. 0x12 instead of 0x10. Interrupt will trigger at 4th byte ( e.g. 0x13) !
 *
 * if one of the registers in 0x00 - 0x0F is used for 16 bit, you try to disable that second register with ios_setRegAccRegisters().
 */
inline void IOSlave::ios_acc_isr()
{
  // get which register interrupted
  uint32_t AccIntStat = IOSLAVEn(_ios_instance)->REGACCINTSTAT;

  // clear the interrupt
  IOSLAVEn(_ios_instance)->REGACCINTCLR = AccIntStat;

#ifdef IOS_DEBUG
  Serial.printf("ios_acc_isr %lx\n", AccIntStat);
#endif

  RecStartReg = 0xFF;
  RecByteCnt = 0;

  // parse the Direct access area
  for (int8_t i = 31 ; i != -1 ; i--) {

    if ( AccIntStat & (1 << i) ) {
      // save starting register
      if (RecStartReg == 0xff) {
        RecStartReg = 31 - i;
      }
      RecByteCnt++;
    }
  }

  /* in AccIntStat
   * bit 31 - 16 represent single byte registers 0x00 - 0xF
   * bit 15 - 0  represent 4 byte registers 0x10 - 0x4f
   * hence the ByteCnt is times 4 in that later area
   * You MUST write on a 4 byte boundery !!
   */
  if (RecStartReg > 0xF) RecByteCnt = RecByteCnt * 4;

#ifdef IOS_DEBUG
  Serial.printf("\tStarting register 0x%lx, number of bytes written %d\n", RecStartReg, RecByteCnt);

  uint8_t *pui8Packet = (uint8_t *) am_hal_ios_pui8LRAM;
  for (uint8_t j = 0; j < RecByteCnt; j++) {
    Serial.printf("\tregister: 0x%x, value: 0x%x\n", RecStartReg+j, pui8Packet[RecStartReg+j]);
  }
#endif

  am_ios_handler();

  // in case an extra call back was subscribed
  if (_onWriteCallback)
  {
    _onWriteCallback(RecStartReg);
  }
}

/**
 * Write register(s) in the direct Access register space
 * @param reg : register between 0x0 - 0x4F
 * @param data : pointer to the data to write
 * @param len : length of data to write
 *
 * return
 * number of data written
 *
 * You can write including Read Only area upto the FIFOBASE
 */
size_t IOSlave::ios_write_reg(uint8_t reg, uint8_t *data, uint8_t len)
{
  uint8_t cnt;
  uint8_t *pui8Packet = (uint8_t *) am_hal_ios_pui8LRAM;

  for (cnt = 0; cnt < len; cnt++) {
    if ((reg + cnt) > _ios_config.ui32FIFOBase) break;
    pui8Packet[reg+cnt] = data[cnt];
  }

  return cnt;
}

/**
 * write to fifo
 * @param data : pointer to the data to write
 * @param len  : length of data to write
 *
 * return
 * number of data written
 */
size_t IOSlave::ios_write_fifo(uint8_t *data, uint8_t len)
{
  uint32_t numWritten = 0, ret;
  ret = am_hal_ios_fifo_write(_ios_handle, data, (uint32_t) len, &numWritten);

  if (ret != AM_HAL_STATUS_SUCCESS){
    Serial.println("error during write fifo");
    return 0;
  }

  return (size_t) numWritten;
}

/**
 * set host interrupt register. Mask selection can be
 *
 * #define AM_HAL_IOS_IOINTCTL_INT0    (0x01)
 * #define AM_HAL_IOS_IOINTCTL_INT1    (0x02)
 * #define AM_HAL_IOS_IOINTCTL_INT2    (0x04)
 * #define AM_HAL_IOS_IOINTCTL_INT3    (0x08)
 * #define AM_HAL_IOS_IOINTCTL_INT4    (0x10)
 * #define AM_HAL_IOS_IOINTCTL_INT5    (0x20)
 */
bool IOSlave::ios_interrupt_host(uint32_t mask)
{
  // Update FIFOCTR for host to read
  am_hal_ios_control(_ios_handle, AM_HAL_IOS_REQ_FIFO_UPDATE_CTR, NULL);

  uint32_t enabled;
  am_hal_ios_control(_ios_handle, AM_HAL_IOS_REQ_HOST_INTEN_GET, &enabled);

  if (! enabled & mask) return false;

  // Interrupt the host
  am_hal_ios_control(_ios_handle, AM_HAL_IOS_REQ_HOST_INTSET, &mask);

  // return false if interrupt was not enabled by host
  if (! enabled & mask) return false;

  return true;
}

/**
 * return number of bytes received in last Host write
 */
int IOSlave::ios_available()
{
  return(RecByteCnt);
}

/**
 * return starting register in last Host write
 */
int IOSlave::ios_get_register()
{
  return(RecStartReg);
}

/**
 * add extra call back from sketch
 */
void IOSlave::ios_onWrite(void (*function)(int))
{
	_onWriteCallback = function;
}

/**
 * Read register(s) in the direct Access register space
 * @param reg : register between 0x0 - 0x4F
 * @param data : pointer to store the data read
 * @param len : length of data to read
 *
 * return
 * True : sucessfull else false
 *
 * You can read including Read Only area upto the FIFOBASE
 */
size_t IOSlave::ios_read_reg(uint8_t reg, uint8_t *data, uint8_t len)
{
  uint8_t cnt;
  uint8_t *pui8Packet = (uint8_t *) am_hal_ios_pui8LRAM;

  // general databyte
  if (reg == 0x80) {
    data[0] = _GenData;
    return 1;
  }

  for (cnt = 0; cnt < len; cnt++) {
    if ((reg + cnt) > _ios_config.ui32FIFOBase) break;
    data[cnt] = pui8Packet[reg+cnt];
  }

  return cnt;
}

/**
 * Read a byte from a register
 * @param reg : register to read
 *
 * return :
 *  register content or 0xff
 *
 * You can write including Read Only area upto the FIFOBASE
 */
uint8_t IOSlave::ios_read_single_reg(uint8_t reg)
{
  uint8_t *pui8Packet = (uint8_t *) am_hal_ios_pui8LRAM;

  // general databyte
  if (reg == 0x80) return _GenData;

  // else if invalid register
  if (reg > _ios_config.ui32FIFOBase) return 0xff;

  return pui8Packet[reg];
}

/**
 * initialize
 * @param config           : set IOS configuration
 * @param EnableFifo       : write to fifo
 * @param enableGeneralCall: detect host writing to address zero (I2C only)
 */
ap3_err_t IOSlave::ios_initialize(am_hal_ios_config_t config, bool EnableFifo, bool enableGeneralCall)
{
  uint32_t retVal32 = 0;
  uint32_t ui32IntMask = 0;
  _ios_config = config;
  _EnableFifo = EnableFifo;
  _enableGeneralCall = enableGeneralCall;

  if (_ios_handle != NULL)
  {
      ios_deinitialize();
  }

  retVal32 = am_hal_ios_initialize(_ios_instance, &_ios_handle);
  if (retVal32 != AM_HAL_STATUS_SUCCESS)
  {
      return AP3_ERR;
  }

  retVal32 = am_hal_ios_power_ctrl(_ios_handle, AM_HAL_SYSCTRL_WAKE, false);
  if (retVal32 != AM_HAL_STATUS_SUCCESS)
  {
      return AP3_ERR;
  }

  retVal32 = am_hal_ios_configure(_ios_handle, &_ios_config);
  if (retVal32 != AM_HAL_STATUS_SUCCESS)
  {
      return AP3_ERR;
  }

  retVal32 = am_hal_ios_enable(_ios_handle);
  if (retVal32 != AM_HAL_STATUS_SUCCESS)
  {
      return AP3_ERR;
  }

  // Register the class into the local for passing to interrupt routine
  ap3_IOS_handle = this;

  // clear any direct access interrupts pending
  IOSLAVEn(_ios_instance)->REGACCINTCLR = AM_HAL_IOS_ACCESS_INT_ALL;

  // clear all IOINT interrupts
  retVal32 = am_hal_ios_interrupt_clear(_ios_handle, AM_HAL_IOS_INT_ALL);
  if (retVal32 != AM_HAL_STATUS_SUCCESS)
  {
      return AP3_ERR;
  }

  if (_EnableFifo) {

    // interrupt on HOST complete (something has been written or read)
    ui32IntMask = AM_HAL_IOS_XCMP_INT;

    ui32IntMask |= AM_HAL_IOS_INT_ERR | AM_HAL_IOS_INT_FSIZE;

    // Set up the IOSINT interrupt pin so we can signal host
    am_hal_gpio_pinconfig(4, g_AM_BSP_GPIO_SLINT);
  }

  // interrupt Generalcall if requested (host writes to I2C address zero)
  if (_enableGeneralCall) ui32IntMask |= AM_HAL_IOS_INT_GENAD;

  if (_EnableFifo || _enableGeneralCall) {
    // enable interrupts in IOS
    retVal32 = am_hal_ios_interrupt_enable(_ios_handle, ui32IntMask);
    if (retVal32 != AM_HAL_STATUS_SUCCESS)
    {
        return AP3_ERR;
    }

    // enable IOS interrupts from the INTEN Register
    NVIC_EnableIRQ(IOSLAVE_IRQn);
  }
  else
  {
    /*
     * WARNING : SLAVEACC AND IOSLAVE_IRQn CAN IMPACT EACH OTHER !!
     * Enable interrupt for all direct registers
     */
    IOSLAVEn(_ios_instance)->REGACCINTEN = AM_HAL_IOS_ACCESS_INT_ALL;
    NVIC_EnableIRQ(IOSLAVEACC_IRQn);
  }

  // Configure the IOM pins. (Must be done by the inherited classes [this is just a reminder])

  return AP3_OK;
}

/**
 * enable interrupts when host has written something to the host
 * registers.
 *
 */
ap3_err_t IOSlave::ios_receiveHostInterrupt()
{
  // can not enable BOTH IOSLAVE and REGACC
  if (! _EnableFifo) return AP3_ERR;

  NVIC_DisableIRQ(IOSLAVE_IRQn);

  uint32_t retVal32 = am_hal_ios_interrupt_clear(_ios_handle, AM_HAL_IOS_INT_ALL);
  if (retVal32 != AM_HAL_STATUS_SUCCESS)
  {
      return AP3_ERR;
  }

  retVal32 = am_hal_ios_interrupt_enable(_ios_handle, AM_HAL_IOS_INT_IOINTW);
  if (retVal32 != AM_HAL_STATUS_SUCCESS)
  {
      return AP3_ERR;
  }

  NVIC_EnableIRQ(IOSLAVE_IRQn);

  return AP3_OK;
}

/**
 * deinitialize IOS and power down
 */
ap3_err_t IOSlave::ios_deinitialize(void)
{
  uint32_t retVal32 = 0;

  if (_ios_handle != NULL)
  {
    retVal32 = am_hal_ios_disable(_ios_handle);
    if (retVal32 != AM_HAL_STATUS_SUCCESS)
    {
        return AP3_ERR;
    }

    retVal32 = am_hal_ios_power_ctrl(_ios_handle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
    if (retVal32 != AM_HAL_STATUS_SUCCESS)
    {
        return AP3_ERR;
    }

    retVal32 = am_hal_ios_uninitialize(_ios_handle);
    if (retVal32 != AM_HAL_STATUS_SUCCESS)
    {
        return AP3_ERR;
    }
  }

  _ios_handle = NULL;
  return AP3_OK;
}

/**
 * IO Peripheral / Slave acc ISR.
 * This is an interrupt handling direct write addresses (0x0 -  0x77)
 */
extern "C" void am_ioslave_acc_isr(void)
{
    ap3_IOS_handle->ios_acc_isr();
}

/**
 * IO Pheripheral / Slave Main ISR.
 * This is an interrupt handling program IO
 */
extern "C" void am_ioslave_ios_isr(void)
{
    ap3_IOS_handle->ios_host_isr();
}

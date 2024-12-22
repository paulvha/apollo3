/*

Copyright (c) 2015 Arduino LLC. All rights reserved.
Copyright (c) 2023 Sfs

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

extern "C"
{
#include <string.h>
}

#include "Ios.h"

AP3_IOS::AP3_IOS(uint8_t ios_instance): IOSlave(ios_instance)
{
	_ios_instance = ios_instance;
}

/**
 * begin communicating peripheral / host SPI mode
 * @param EnableFiFo : true => use FIFO, false => use direct register
 *
 */
void AP3_IOS::beginSPI(bool EnableFifo)
{
  // Configure pins for SPI interface
  am_bsp_ios_pins_enable(_ios_instance, AM_HAL_IOS_USE_SPI);

	ap3_err_t retval = AP3_OK;

	// Configure the IOS in SPI mode.
  _PeConfig.ui32InterfaceSelect = AM_HAL_IOS_USE_SPI;

	// Eliminate the "read-only" section, so an external host can use the
	// entire "direct access" section.
	_PeConfig.ui32ROBase = 0x78; 	// => 120 / 8 = 15 => 0x0F,

	// Set the FIFO base to the maximum value, making the "direct write"
	// section as big as possible.
	_PeConfig.ui32FIFOBase = 0x80; // 128 / 8 = 16 => 0x10

	// We don't need any RAM space, so extend the FIFO all the way to the end
	// of the LRAM.
	_PeConfig.ui32RAMBase = 0x100; // 256 / 8 = 32 => 0x20

	// FIFO Threshold - set to half the size
	_PeConfig.ui32FIFOThreshold = 0x40;

	if (EnableFifo) {
		_PeConfig.pui8SRAMBuffer = TxFifoBuffer;
		_PeConfig.ui32SRAMBufferCap = AM_IOS_TX_BUFSIZE_MAX;
	}
	else {
		_PeConfig.pui8SRAMBuffer = 0;
		_PeConfig.ui32SRAMBufferCap = 0;
	}

	retval = ios_initialize(_PeConfig, EnableFifo, false); // Initialize the IOS
	if (retval != AP3_OK)
	{
		Serial.printf("Error during ios_intialize\n");
	}
}

/**
 * begin communicating peripheral / host I2C mode
 * @param EnableFiFo : true => use FIFO, false => use direct register
 * @param enableGeneralCall " true -> detect host sending on I2Caddress 0
 */
void AP3_IOS::begin(uint8_t address, bool EnableFifo, bool enableGeneralCall)
{
	// Configure pins for I2C interface
  am_bsp_ios_pins_enable(_ios_instance, AM_HAL_IOS_USE_I2C);

	ap3_err_t retval = AP3_OK;

	// Configure the IOS in I2C mode.
  _PeConfig.ui32InterfaceSelect = AM_HAL_IOS_USE_I2C | AM_HAL_IOS_I2C_ADDRESS(address << 1);

	// Eliminate the "read-only" section, so an external host can use the
	// entire "direct access" section.
	_PeConfig.ui32ROBase = 0x78; 	// => 120 / 8 = 15 => 0x0F,

	// Set the FIFO base to the maximum value, making the "direct write"
	// section as big as possible.
	_PeConfig.ui32FIFOBase = 0x80; // 128 / 8 = 16 => 0x10

	// We don't need any RAM space, so extend the FIFO all the way to the end
	// of the LRAM.
	_PeConfig.ui32RAMBase = 0x100; // 256 / 8 = 32 => 0x20

	// FIFO Threshold - set to half the size
	_PeConfig.ui32FIFOThreshold = 0x40;

	if (EnableFifo) {
		_PeConfig.pui8SRAMBuffer = TxFifoBuffer;
		_PeConfig.ui32SRAMBufferCap = AM_IOS_TX_BUFSIZE_MAX;
	}
	else {
		_PeConfig.pui8SRAMBuffer = 0;
		_PeConfig.ui32SRAMBufferCap = 0;
	}

	retval = ios_initialize(_PeConfig, EnableFifo, enableGeneralCall); // Initialize the IOS
	if (retval != AP3_OK)
	{
		Serial.printf("Error during ios_intialize\n");
	}
}

void AP3_IOS::end()
{
	ios_deinitialize(); //De init and power down ios

	// reset pins
	if (_PeConfig.ui32InterfaceSelect == AM_HAL_IOS_USE_SPI)
		am_bsp_ios_pins_disable(_ios_instance, AM_HAL_IOS_USE_SPI);
	else
		am_bsp_ios_pins_disable(_ios_instance, AM_HAL_IOS_USE_I2C);
}

/**
 * In FIFO mode, use the HOST registers to raise an interrupt on
 * on the interrupt pin
 *
 * return :
 * True if OK
 * False if interrupt was not enabled by the HOST
 */
bool AP3_IOS::set_interrupt_host(uint32_t mask)
{
	return(ios_interrupt_host(mask));
}

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
size_t AP3_IOS::write(uint8_t *data, size_t quantity, bool UseFifo)
{
	if (UseFifo){
		//disgard register to write
		return(ios_write_fifo(&data[1], quantity-1) + 1);
	}

	uint8_t reg = data[0];
	return(ios_write_reg(reg, &data[1], quantity-1) + 1);
}

/**
 * check any bytes available from host
 */
int AP3_IOS::available(void)
{
	return _rxBuffer.available();
}

/**
 * read bytes from host
 * the first byte is the starting register that was written by the host
 * next bytes is the data written
 */
int AP3_IOS::read(void)
{
	return _rxBuffer.read_char();
}

int AP3_IOS::peek(void)
{
	return _rxBuffer.peek();
}

/**
 * empty rx buffer
 */
void AP3_IOS::flush(void)
{
	_rxBuffer.clear();
}

/**
 * set from sketch to be called back when data has been received
 */
void AP3_IOS::onReceive(void (*function)(int))
{
	_onReceiveCallback = function;
}

// called after all bytes have been received as slave
void AP3_IOS::onService()
{
	int reg = ios_get_register();

	_rxBuffer.clear();

	// Wire.read can be used by the sketch
	// first entry is register
	_rxBuffer.store_char(reg);

	// Copy the bytes into the rx buffer
	int cnt = ios_available();
	for (uint8_t byteRead = 0; byteRead < cnt; byteRead++)
	{
		_rxBuffer.store_char(ios_read_single_reg(reg+byteRead) ); // Read data
	}

	// call on onReceive (set by sketch)
	if (_onReceiveCallback)
	{
		_onReceiveCallback(cnt);
	}
}

/**
	// for IOS...
	extern void am_ios_handler(void)          __attribute ((weak));
	#define IOS_IT_HANDLER am_ios_handler
 */
AP3_IOS IOS(0);

/**
 * this will enable the ap3_ioslave.cpp to call the onService
 * routine as a call back
 */
void IOS_IT_HANDLER(void) {
    IOS.onService();
}

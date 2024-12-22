/**
 * SoftWire - Version 1.0 / paulvha / February 2019
 *
 * This is a bit-banging I2C implemenation that is taken from the ESP8266.
 * It has been adjusted to work on an ESP32 and support clock-stretching.
 *
 * The hardware I2C on an ESP32 is known for NOT supporting clock stretching.
 *
 * While it is aimed and tested for the SCD30, it should work for other
 * devices on the ESP32 as well.
 *
 * Below the original heading on the ESP8266
 * 
 * December 2024 / paulvha : Special port for MLX90614
 * Tested on UNOR4 & Artemis ATP V 1.2.3
 */

/*
  twi.h - Software I2C library for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <defwire.h>
#if defined(MLX_SOFTWIRE)

#ifndef SI2C_h
#define SI2C_h

#include "Arduino.h"

#define I2C_OK                      0
#define I2C_SCL_HELD_LOW            1
#define I2C_SCL_HELD_LOW_AFTER_READ 2
#define I2C_SDA_HELD_LOW            3
#define I2C_SDA_HELD_LOW_AFTER_INIT 4

class TWI
{
	public:
		TWI();
		void twi_init(unsigned char sda, unsigned char scl);
		void twi_stop(void);
		void twi_setClock(unsigned int freq);
		void twi_setClockStretchLimit(uint32_t limit);
		uint8_t twi_writeTo(unsigned char address, unsigned char * buf, unsigned int len, unsigned char sendStop);
		uint8_t twi_readFrom(unsigned char address, unsigned char * buf, unsigned int len, unsigned char sendStop);
		uint8_t twi_status();
	
	private :

};

void twi_recovery();
static bool twi_write_stop(void);
static bool twi_write_start(void); 
static bool twi_write_bit(bool bit);
static bool twi_read_bit(void);

#endif  // SI2C_h
#endif  // SOFTWIRE



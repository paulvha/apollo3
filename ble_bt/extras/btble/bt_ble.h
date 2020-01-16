/*
 *  paulvha / January 2020 version 1.0
 *
 *  This is extreem stripdown version of gatttool and it will only read
 *  the battery level, set the battery resistor and read temperature in celsius
 *  or Fahrenheit from a remote device.
 *
 *  This has been created and tested to work against Apollo3 running ble_bt.ino
 *
 *******************************************************************************
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011  Nokia Corporation
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

enum RequestValue {
    NOVALUE = 0,
    BATTERY,
    BATTERYLOAD,
    TEMPCELSIUS,
    TEMPFAHRENHEIT
};

/*! battery load generated with https://www.uuidgenerator.net/version4
* fc8b661f-86d5-4e33-97a4-b3fef6772838 */
#define  BATT_UUID_CHR_CONFIG_BASE     "fc8b661f-86d5-4e33-97a4-b3fef6772838"

GIOChannel *gatt_connect(const char *src, const char *dst,
            const char *dst_type, const char *sec_level,
            int psm, int mtu, BtIOConnect connect_cb,
            GError **gerr);
size_t gatt_attr_data_from_string(const char *str, uint8_t **data);

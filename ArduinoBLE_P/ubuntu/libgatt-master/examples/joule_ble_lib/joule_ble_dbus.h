/*
 * joule_ble_dbus.h
 *
 *  Created on: Apr 12, 2018
 *      Author: whizz
 */

#ifndef EXAMPLES_JOULE_BLE_LIB_JOULE_BLE_DBUS_H_
#define EXAMPLES_JOULE_BLE_LIB_JOULE_BLE_DBUS_H_

#include "gattlib.h"

int read_characteristic_by_uuid(gatt_connection_t*, char* uuid, char* buffer, int* len);
int write_characteristic_by_uuid(gatt_connection_t*, char* uuid, char* buffer, int len);



#endif /* EXAMPLES_JOULE_BLE_LIB_JOULE_BLE_DBUS_H_ */

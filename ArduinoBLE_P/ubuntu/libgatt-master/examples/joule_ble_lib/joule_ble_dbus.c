/*
 * joule_ble_dbus.c
 *
 *  Created on: Apr 12, 2018
 *      Author: whizz
 */

#include "joule_ble_dbus.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int read_characteristic_by_uuid(gatt_connection_t* connection, char* uuid, char* buffer, int* len)
{
	uuid_t g_uuid;
	int ret, i;
	//char buffer[256];
	//int len = 256;
	printf("UUID: %s\n", uuid);
	printf("Length received: %d\n", *len);
	printf("Connection received: 0x%X\n", connection);
	if(connection == NULL)
		return -1;
	if (gattlib_string_to_uuid(uuid, strlen(uuid) + 1, &g_uuid) < 0) {
		return -1;
	}

	ret = gattlib_read_char_by_uuid(connection, &g_uuid, (void*)buffer, len);
	if(ret == 0)
	{
		printf("Read UUID completed: %d\n", *len);
		for (i = 0; i < *len; i++)
			printf("%02x ", buffer[i]);
		printf("\n");
		return 0;
	}

	return -1;
}

int write_characteristic_by_uuid(gatt_connection_t* connection, char* uuid, char* buffer, int len)
{
	uuid_t g_uuid;
	int i;
	//char buffer[256];
	//int len = 256;
	printf("UUID: %s\n", uuid);
	printf("Length received: %d\n", len);
	printf("Connection received: 0x%X\n", connection);
	if(connection == NULL)
		return -1;
	if (gattlib_string_to_uuid(uuid, strlen(uuid) + 1, &g_uuid) < 0) {
		return -1;
	}
	return gattlib_write_char_by_uuid(connection, &g_uuid, buffer, len);

}





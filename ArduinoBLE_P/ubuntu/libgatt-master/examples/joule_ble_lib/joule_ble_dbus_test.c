/*
 * joule_ble_dbus_test.c
 *
 *  Created on: Apr 12, 2018
 *      Author: whizz
 */

#include "joule_ble_dbus.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	char buffer[256];
	int i, ret;
	size_t len;
	gatt_connection_t* connection=NULL;
	connection = gattlib_connect(NULL, "00:0B:57:29:FC:F9", BDADDR_LE_PUBLIC, BT_SEC_LOW, 0, 0);
	if (connection == NULL) {
		fprintf(stderr, "Fail to connect to the bluetooth device.\n");
		return 1;
	}
	len = sizeof(buffer);
	ret = read_characteristic_by_uuid(connection, "34596dce-6442-4903-a78e-81635ca92f00", buffer, &len);
	printf("Read UUID completed: %d\n", len);
	for (i = 0; i < len; i++)
		printf("%02x ", buffer[i]);

	printf("\n");
	return 0;
}



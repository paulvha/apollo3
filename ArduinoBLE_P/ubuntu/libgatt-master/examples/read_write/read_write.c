/*
 *
 *  GattLib - GATT Library
 *
 *  Copyright (C) 2016-2017  Olivier Martin <olivier@labapart.org>
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "gattlib.h"

typedef enum { READ, WRITE} operation_t;
operation_t g_operation;

static uuid_t g_uuid;
long int value_data;

static void usage(char *argv[]) {
	printf("%s <device_address> <read|write> <uuid> [<hex-value-to-write>]\n", argv[0]);
}

int main(int argc, char *argv[]) {
/*
	char buffer[256];
	int i, ret;
	size_t len;
	gatt_connection_t* connection1=NULL;
	gatt_connection_t* connection2=NULL;
	char uuid[] = "34596dce-6442-4903-a78e-81635ca92f00";
	connection1 = gattlib_connect(NULL, "00:0B:57:29:FC:F9", BDADDR_LE_PUBLIC, BT_SEC_LOW, 0, 0);
	if (connection1 == NULL) {
		fprintf(stderr, "Fail to connect to the bluetooth device.\n");
		return 1;
	}
	printf("Connection1: 0x%X\n", connection1);
	connection2 = gattlib_connect(NULL, "00:0B:57:0B:DF:D1", BDADDR_LE_PUBLIC, BT_SEC_LOW, 0, 0);
	if (connection2 == NULL) {
		fprintf(stderr, "Fail to connect to the bluetooth device.\n");
		return 1;
	}
	printf("Connection2: 0x%X\n", connection2);
	if (gattlib_string_to_uuid(uuid, strlen(uuid) + 1, &g_uuid) < 0) {
			return -1;
		}
	len = sizeof(buffer);
	ret = gattlib_read_char_by_uuid(connection1, &g_uuid, buffer, &len);
	printf("Read UUID completed1: %d\n", len);
	for (i = 0; i < len; i++)
		printf("%02x ", buffer[i]);

	printf("\n");

	len = sizeof(buffer);
	ret = gattlib_read_char_by_uuid(connection2, &g_uuid, buffer, &len);
	printf("Read UUID completed2: %d\n", len);
	for (i = 0; i < len; i++)
		printf("%02x ", buffer[i]);

	printf("\n");
	gattlib_disconnect(connection1);
	gattlib_disconnect(connection2);
	return 0;*/
	uint8_t buffer[100];
	int i, ret;
	size_t len;
	gatt_connection_t* connection=NULL;


	if ((argc != 4) && (argc != 5)) {
		usage(argv);
		return 1;
	}

	if (strcmp(argv[2], "read") == 0) {
		g_operation = READ;
	} else if ((strcmp(argv[2], "write") == 0) && (argc == 5)) {
		g_operation = WRITE;

		if ((strlen(argv[4]) >= 2) && (argv[4][0] == '0') && (argv[4][0] == 'x')) {
			value_data = strtol(argv[4], NULL, 0);
		} else {
			value_data = strtol(argv[4], NULL, 16);
		}
		printf("Value to write: 0x%lx\n", value_data);
	} else {
		usage(argv);
		return 1;
	}

	if (gattlib_string_to_uuid(argv[3], strlen(argv[3]) + 1, &g_uuid) < 0) {
		usage(argv);
		return 1;
	}
	printf("UUID: %X\n", g_uuid.value);
	while (connection == NULL)
		connection = gattlib_connect(NULL, argv[1], BDADDR_LE_PUBLIC, BT_SEC_LOW, 0, 0);
	/*if (connection == NULL) {
		fprintf(stderr, "Fail to connect to the bluetooth device.\n");
		return 1;
	}
	sleep(10);*/
	if (g_operation == READ) {
		len = sizeof(buffer);
		ret = gattlib_read_char_by_uuid(connection, &g_uuid, buffer, &len);
		assert(ret == 0);

		printf("Read UUID completed: ");
		for (i = 0; i < len; i++)
			printf("%02x ", buffer[i]);

		printf("\n");

	} else {
		buffer[0]=0x24;
		buffer[1]=0x56;
		ret = gattlib_write_char_by_uuid(connection, &g_uuid, buffer, 2);
		printf("Return value: %d\n", ret);
		assert(ret == 0);
	}

	gattlib_disconnect(connection,true);
	return 0;
}

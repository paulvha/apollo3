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
 *  ============================================================================
 *  paulvha / November 2022 / version 1.0 :
 *
 *  based on the example notify.c, created an extended version
 *  to work with the example13_ph_mtu_size peripheral sketch that is part of ArduinoBLE_P
 *
 *  Many modern processor chips support BLE with integrated HCI-layer. The layers above (L2CAP, ATT, GP, GATT
 *  etc) are handled by software / drivers. The default MTU size is 23, this is the minimum by standard.
 *  BUT it could be increased. Especially when you do transfer of large data this can be interesting.
 *
 *  This central will connect to the peripheral. On the peripheral you can observe a change in MTU connection.
 *  with the current dbus only a server (peripheral) would be able to get the MTU size based with AcquireWrite or AcquireNotify.
 *
 *  In this setup the peripheral will share this with the central
 *
 *  You do not have provide the BLE address, just the name of the device as advertised
 *
 */

# include <assert.h>
# include <glib.h>
# include <stdio.h>
# include <stdlib.h>
# include <signal.h>
# include <getopt.h>
# include "gattlib.h"

// start of notification
#define MAGICNUM 0xCF

// Battery Level UUID
const uuid_t g_battery_level_char_uuid = CREATE_UUID16(0x2A19);
//const uuid_t g_battery_level_serv_uuid = CREATE_UUID16(0x180F);

#define BLE_SCAN_TIMEOUT   10				// default scan in seconds
char *default_dev_name = "BatteryMonitor";	// default devicename

gatt_connection_t* connection = NULL;
GMainLoop* loop = NULL;
void* adapter = NULL;
char* dev_addr = NULL;
char* dev_name = NULL;
int timeout = BLE_SCAN_TIMEOUT;				// set default timeout

/**
 * call back if device is discovered
 * if dev_name than retrieve the BLE address
 */
static void ble_discovered_device(const char* addr, const char* name) {

	if (name) {
		// get address of the device we are looking for
		if (strcmp(name,dev_name) == 0) {
			fprintf(stderr, "FOUND %s on '%s'\n",name, addr);
			dev_addr = strdup(addr);
		}
		else
			fprintf(stderr,"Discovered %s - '%s'\n",name, addr);

	} else
		fprintf(stderr,"Discovered '%s'\n", addr);
}

/**
 * scan for the device dev_name and store the address
 */
int Find_device(){
	int ret;

	ret = gattlib_adapter_open(NULL, &adapter);
	if (ret) {
		fprintf(stderr, "ERROR: Failed to open adapter.\n");
		return -1;
	}

	fprintf(stderr,"Start scanning for %s for %d seconds\n", dev_name, timeout);

	ret = gattlib_adapter_scan_enable(adapter, ble_discovered_device, timeout);

	if (ret) {
		fprintf(stderr, "ERROR: Failed to scan.\n");
		return -1;
	}

	gattlib_adapter_scan_disable(adapter);

	fprintf(stderr,"Scan completed\n");

	return 0;
}

/**
 * This will be called when a notification is arriving
 *
 * byte 0 Magicnum
 * byte 1 MTU
 * byte 2 battery percentage
 */
void notification_handler(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) {

  static uint8_t mtu =0;
	if (data_length != 3 || data[0] != MAGICNUM) {
		printf("Invalid notifcation package\n");
		return;
	}

	if (mtu != data[1]) {
		printf("Current MTU size %d\n", data[1]);
		mtu = data[1];
	}

	printf("Current battery percentage %d%%\n", data[2]);

	/** below left in for debug */
	//printf("Notification Handler: ");
/*
	// add the UUID
	char uuid_str[MAX_LEN_UUID_STR + 1];
	gattlib_uuid_to_string(uuid, uuid_str, sizeof(uuid_str));
	printf("UUID %s: ", uuid_str);
*/
	//gattlib_rssi(connection);

/*
	// display data raw
	int i;
	for (i = 0; i < data_length; i++) {
		printf("%02x ", data[i]);
	}
	printf("\n");
*/
}

typedef struct {
	GIOChannel*               io;
	GAttrib*                  attrib;

	// We keep a list of characteristics to make the correspondence handle/UUID.
	gattlib_characteristic_t* characteristics;
	int                       characteristic_count;

	// keep a disconnection pointer just in case
	 gatt_disconnect_cb_t *disconnect_cb;
} gattlib_context_t;

/**
 * usage
 */
static void usage(char *argv[]) {
	fprintf(stderr, "%s [-d devicename] [-a adapter] [-t scan timeout]\n", argv[0]);
	fprintf(stderr, "default devicename   %s\n", default_dev_name);
	fprintf(stderr, "default scan timeout %d seconds\n", timeout);
	exit(-1);
}

/**
 * close out correctly
 */
void cleanup(int ret) {
	// if called before loop was started skip
	if (loop == NULL) {
		// check that connection needs to be cancelled
		if (connection != NULL) {
			// this can cause a warning that can be ignored
			// GLib-GObject-CRITICAL **: 16:37:55.553: g_signal_handler_disconnect: assertion 'handler_id > 0' failed
			gattlib_disconnect(connection,true);
		}
		fprintf(stderr,"done\n");
		exit(ret);
	}
	else
		 g_main_loop_quit(loop);
}

/**
 * This will be called when peripheral disconnected
 */
void disconnect_handler(gatt_connection_t* connection)
{
  fprintf(stderr,"Peripheral disconnected.\n");
  cleanup(6);
}

/**
 * this is called when contrl-C is pressed on the keyboard
 */
void my_handler(int s){
   //fprintf(stderr,"Caught signal %d\n", s);
   cleanup(0);
}

/**
 * here the joy begins
 */
int main(int argc, char *argv[]) {
	int ret, opt;

	// capture cntrl-c
	signal (SIGINT,my_handler);

	/* parse commandline */
    while ((opt = getopt(argc, argv, "a:d:t:")) != -1)
    {
		switch (opt) {
			case 'a':   // adapter
				adapter = strdup(optarg);
				break;

			case 'd':	// devicename to look for
				dev_name = strdup(optarg);
				break;

			case 't':	// set scan timeout
				timeout = (int) strtod(optarg, NULL);
				break;

			default:
				fprintf(stderr, "Unknown option %d\n", opt);
				usage(argv);
				break;
		}
    }

	// check for devicename else use default
	if (dev_name == NULL) dev_name = strdup(default_dev_name);

	// look for device by name
	ret = Find_device();
	if (ret) exit(1);

	if (dev_addr == NULL){
		fprintf(stderr,"Device with name %s, was NOT discovered\n", dev_name);
		exit(2);
	}

	fprintf(stderr,"Trying to connect\n");

	connection = gattlib_connect(NULL, dev_addr, BDADDR_LE_PUBLIC, BT_SEC_LOW, 0, 45);
	if (connection == NULL) {
		fprintf(stderr,"Fail to connect to the bluetooth device. %s\n",dev_name);
		exit(3);
	}

	fprintf(stderr,"connected\n");

  // register disconnect call back
  ret = gattlib_register_disconnect(connection, disconnect_handler);
  if (ret) {
    fprintf(stderr, "Fail to set disconnect callback.\n");
    cleanup(4);
  }

	// set notification call back
	gattlib_register_notification(connection, notification_handler, NULL);

	// enable notifications
	ret = gattlib_notification_start(connection, &g_battery_level_char_uuid);
	if (ret) {
		fprintf(stderr, "Fail to start notification\n.");
		cleanup(4);
	}

	fprintf(stderr, "Started notification\n.");

	loop = g_main_loop_new(NULL, 0);
	g_main_loop_run(loop);
	fprintf(stderr, "loop ended\n");
	g_main_loop_unref(loop);
	loop = NULL;
	cleanup(0);
}

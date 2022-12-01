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
 *
 *  paulvha / November 2022 / version 1.0:
 *
 *  Based on the example notification.c, created an extended version
 *  to work with the BME280 peripheral sketch that is part of ArduinoBLE_P.
 *
 *  It will receive the measured data over notification, broken down in blocks of max 20 bytes.
 *  The different blocks are combined to a single buffer and then put forward to display
 *  the results.
 *
 *  You can provide the BLE address OR just the devicename as advertised. For all options
 *  see usage() or use the '-h'  option with the binary
 *
 *  It will take keyboard input to be send to the BME280 sketch to change some parameters
 *  To exit : q + <enter> or press 'control + C'
 *
 *  All progress and error message are printed to stderr, where the BME280 results are printed to stdout.
 *  This is done in case you want to capure the measured data only. (e.g. comma separated)
 */

# include <assert.h>
# include <glib.h>
# include <stdio.h>
# include <stdlib.h>
# include <signal.h>
# include <getopt.h>
# include "gattlib.h"
# include <stdbool.h>
# include <time.h>

/***********************************************************************/
// Define here the structure for the data to exchange.
// This structure must be defined the same on the central and peripheral
/***********************************************************************/
typedef struct{
  // BME280 data
  float humidity;
  float pressure;
  float altitude;
  float temperature;
  uint8_t meter;       // if true altitude is in meter, else in feet
  uint8_t celsius;     // if true temperature is celsius, else fahrenheit
} data_to_exchange;

union {
	data_to_exchange p;
	uint8_t	v[sizeof(data_to_exchange)];
} val;

// To improve synchronization there are 2 checks:
#define MAGICNUM 0xCF						// should be byte 0 in a new message
#define MAGICLEN sizeof(data_to_exchange)	// should be byte 1 (length) in new message

// BME280 UUID's
char *BME280Service = "19B10010-E8F2-537E-4F6C-D104768A1214";				// not really used (yet), info for now
char *BME280sCharacteristic = "19B10011-E8F2-537E-4F6C-D104768A1214";		// this is receive on peripheral for commands
char *BME280rCharacteristic = "19B10012-E8F2-537E-4F6C-D104768A1214";		// this is send on peripheral (notify)

// default devicename if NO BLE address or devicename provided on command line
char *default_dev_name = "Artemis peripheral BME280 BLE";

static uuid_t s_uuid, r_uuid;
gatt_connection_t* connection = NULL;
GMainLoop* loop = NULL;
void* adapter = NULL;
char* dev_addr = NULL;
char* dev_name = NULL;
char* FileName = NULL;		// filename to store received data
bool dev_discovered = false;	// true = peripheral device is detected
bool TimeStamp = true;			// true = add timestamp to output
bool DisplayCommaSep = false;	// true = display comma separated
bool SHOW_scan = true;			// true = show all scan discovered devices
int timeout = 10;				// default scantime in seconds

/**
 *  generate timestamp
 */
void time_stamp(char *buf)
{
    time_t ltime;
    struct tm *tm ;

    ltime = time(NULL);
    tm = localtime(&ltime);

    static const char wday_name[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

    static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	if (DisplayCommaSep) {
		sprintf(buf, "%.3s,%.3s%3d,%.2d:%.2d:%.2d,%d",
	    wday_name[tm->tm_wday],  mon_name[tm->tm_mon],
	    tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
	    1900 + tm->tm_year);
	}
    else {
		sprintf(buf, "%.3s %.3s%3d %.2d:%.2d:%.2d %d",
	    wday_name[tm->tm_wday],  mon_name[tm->tm_mon],
	    tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
	    1900 + tm->tm_year);
	}
}

/**
 * Display valid command options to send to BME280 sketch
 */
void display_menu()
{
	fprintf(stderr,"1.  Request altitude in meters\n");
	fprintf(stderr,"2.  Request altitude in feet\n");
	fprintf(stderr,"3.  Request temperature in Celsius\n");
	fprintf(stderr,"4.  Request temperature in Fahrenheit\n");
	fprintf(stderr,"5.  Request STOP sending data\n");
	fprintf(stderr,"6.  Request (re)START sending data\n");
	fprintf(stderr,"7.  Request NOW sending data\n");
	fprintf(stderr,"q.  Quit this program\n\n");
}

/**
 * save received BME280 data to file
 */
void SaveToFile()
{
	FILE * pFile;
	char tm[30];

	// open file append
	pFile=fopen(FileName, "a");

	if( ! pFile ) {
		fprintf(stderr, "Error opening file: %s.", FileName);
		return;
	}

	// time stamp needed
	if (TimeStamp) time_stamp(tm);

	if (DisplayCommaSep) {

		// add timestamp ?
		if (TimeStamp) fprintf(pFile, "%s,",tm);

		fprintf(pFile,"%2.2f,", val.p.temperature);
		if (val.p.celsius) fprintf(pFile,"*C,");
		else fprintf(pFile,"*F,");

		fprintf(pFile,"%2.2f,%%,%2.2f,hPa,", val.p.humidity, val.p.pressure);

		fprintf(pFile,"%2.2f,", val.p.altitude);
		if (val.p.meter) fprintf(pFile,"Meter\n");
		else fprintf(pFile,"Feet\n");

	}
	else {
		// add timestamp ?
		if (TimeStamp) fprintf(pFile,"\n%s\n",tm);

		fprintf(pFile,"\nTemperature\t%2.2f", val.p.temperature);
		if (val.p.celsius) fprintf(pFile,"*C\n");
		else fprintf(pFile,"*F\n");

		fprintf(pFile,"Humidity\t%2.2f %%\n", val.p.humidity);
		fprintf(pFile,"Pressure\t%2.2f hPa\n", val.p.pressure);

		fprintf(pFile,"Altitude\t%2.2f", val.p.altitude);
		if (val.p.meter) fprintf(pFile," Meter\n");
		else fprintf(pFile," Feet\n");
	}

	// add EOL
	fprintf(pFile,"\n");

	// close file
	fclose(pFile);
}
/**
 * Display BME280 results
 */
void display_BME280()
{
	int  i;
	char tm[30];

	// check for all zero's
	for (i = 0 ; i < sizeof(val.p); i++) {
		if (val.v[i] != 0) break;
	}

	// if all zero's
    if (i == sizeof(val.p)) {
		if (! DisplayCommaSep)
			fprintf(stderr, "No valid data received.\n");
		return;
	}

	// add timestamp ?
	if (TimeStamp) time_stamp(tm);

	// Display comma separated
	if (DisplayCommaSep) {

		// add timestamp ?
		if (TimeStamp) printf("%s,",tm);

		printf("%2.2f,", val.p.temperature);
		if (val.p.celsius) printf("*C,");
		else printf("*F,");

		printf("%2.2f,%%,%2.2f,hPa,", val.p.humidity, val.p.pressure);

		printf("%2.2f,", val.p.altitude);
		if (val.p.meter) printf("Meter\n");
		else printf("Feet\n");
		return;
	}

	// add timestamp ?
	if (TimeStamp) printf("\n%s\n",tm);

	printf("\nTemperature\t%2.2f", val.p.temperature);
	if (val.p.celsius) printf("*C\n");
	else printf("*F\n");

	printf("Humidity\t%2.2f %%\n", val.p.humidity);
	printf("Pressure\t%2.2f hPa\n", val.p.pressure);

	printf("Altitude\t%2.2f", val.p.altitude);
	if (val.p.meter) printf(" Meter\n");
	else printf(" Feet\n");

	// Do we need to save to file ?
	if (FileName) SaveToFile();
}

/**
 * call back if device is discovered
 *
 * if dev_name provided, try to retrieve the BLE address
 * if dev_addr provided, check device can be found
 */
static void ble_discovered_device(const char* addr, const char* name) {

	// If device is not discovered, but we have a dev_addr
	// this address was provided on the command line.
	// We check for match to make sure the peripheral is in reach.
	if (! dev_discovered && dev_addr != NULL) {

		if (strcmp(addr,dev_addr) == 0) {
			if (name) fprintf(stderr, "FOUND '%s': '%s'.\n", addr, name);
			else fprintf(stderr, "FOUND device '%s'.\n", addr);
			dev_discovered = true;
			return;
		}
	}

	// if scanned device has a name
	if (name) {

		// if devicename was provided and not found yet
		if (dev_name && ! dev_discovered) {
			// try to get BLE address of the devicename we are looking for
			if (strcmp(name,dev_name) == 0) {
				fprintf(stderr, "FOUND '%s' on '%s'.\n",name, addr);
				dev_addr = strdup(addr);
				dev_discovered = true;
			}
		}
		else if (SHOW_scan)
			fprintf(stderr,"Discovered %s - '%s'.\n",name, addr);

	} else if (SHOW_scan)
		fprintf(stderr,"Discovered '%s'.\n", addr);
}

/**
 * Start scanning for the device either based on provided BLE address
 * or else look for dev_name and store the BLE the address
 */
int Find_device(){
	int ret;

	ret = gattlib_adapter_open(NULL, &adapter);
	if (ret) {
		fprintf(stderr, "ERROR: Failed to open adapter.\n");
		return -1;
	}

	if (dev_name) fprintf(stderr,"Start scanning for '%s' (%d seconds)\n\n", dev_name, timeout);
	else fprintf(stderr,"Start scanning for '%s' (%d seconds)\n\n", dev_addr, timeout);

	ret = gattlib_adapter_scan_enable(adapter, ble_discovered_device, timeout);

	if (ret) {
		fprintf(stderr, "ERROR: Failed to scan.\n");
		return -1;
	}

	gattlib_adapter_scan_disable(adapter);

	fprintf(stderr,"Scan completed.\n");

	return 0;
}

/**
 * This will be called when a notification is arriving
 */
void notification_handler(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data)
{
	static uint8_t len = 0;
	static uint8_t offset;
	int i = 0;

#if (0)
	/***** below left in for debug ****/
	printf("Notification Handler: ");

	// add the UUID
	char uuid_str[MAX_LEN_UUID_STR + 1];
	gattlib_uuid_to_string(uuid, uuid_str, sizeof(uuid_str));
	printf("UUID %s: ", uuid_str);

	// DEBUG : display data raw
	for (int j = 0; j < data_length; j++) {
		printf("%02x ", data[j]);
	}
	printf("\n");

	//gattlib_rssi(connection);
    /********* end of debug ***********/
#endif

	if (len == 0) {
		// check for correct packet start to improve synchronisation
		if (data[0] != MAGICNUM || data[1] != MAGICLEN ) return;

		len = data[1];	// capture length
		offset = 0;		// reset the current storage position
		i = 2;			// skep magicnum & length byte from dataset
	}

	// capture received data
	for (; i < data_length; i++) val.v[offset++] = data[i];

	// do we have all bytes (broken in blocks to fit MTU by sender)
	if (offset == len) {
		display_BME280();

		// reset for next message
		len = offset = 0;
	}
}

/**
 * usage
 */
static void usage(char *argv[]) {
	fprintf(stderr, "%s  [-a adapter] [-b address] [-d devicename] [-t scan timeout] [-s] [-r]\n", argv[0]);
	fprintf(stderr, "-a  option to provide local BLE adapter name. \n");
	fprintf(stderr, "-b  provide the remote BLE address instead of name.\n");
	fprintf(stderr, "-c  display BME280 data comma separated.\n");
	fprintf(stderr, "-d  provide the remote BLE devicename instead of address.\n");
	fprintf(stderr, "-f  provide the filename to store received Ascii strings.\n");
	fprintf(stderr, "-r  remove timestamp from BME280 data.\n");
	fprintf(stderr, "-s  will only display scan result for device found.\n");
	fprintf(stderr, "-t  set scan timeout (default %d seconds)\n", timeout);
	fprintf(stderr, "\nif NO device name and no address the default devicename is '%s'\n", default_dev_name);

	exit(-1);
}

/**
 * close out correctly
 * @param ret : the exit value
 */
void cleanup(int ret) {

	// if called when loop is not active
	if (! loop) {

		// check that a connection needs to be cancelled
		if (connection) {

			// this can cause a warning that can be ignored
			gattlib_disconnect(connection,true);
		}

		fprintf(stderr,"Bye, exit code: %d\n", ret);
		exit(ret);
	}
	else // stop loop first
		g_main_loop_quit(loop);
}

/**
 * This will be called when peripheral disconnected
 */
void disconnect_handler(gatt_connection_t* connection) {
	fprintf(stderr,"Peripheral disconnected.\n");
	cleanup(6);
}

/**
 * this is called when contrl-C is pressed on the keyboard
 */
void my_signal_handler(int s){
   //fprintf(stderr,"Caught signal %d\n", s);
   cleanup(0);
}

/**
 * Process keyboard input
 */
static gboolean
handle_keyboard (GIOChannel * source, GIOCondition cond)
{
	gchar *str = NULL;

	if (g_io_channel_read_line (source, &str, NULL, NULL,
	  NULL) != G_IO_STATUS_NORMAL) {
		return TRUE;
	}

	// requesting to stop
	if (str[0] == 'q' || str[0] == 'Q') {
		cleanup(0);
	}
	// if only <enter>
	else if (str[0] == 0x0a) {
		display_menu();
	}
	// otherwise send it
	else {
		int ret = gattlib_write_char_by_uuid(connection, &s_uuid, str, 1);

		if (ret) {
			fprintf(stderr, "Failed to send keyboard data.\n");
			//cleanup(4);
		}
	}

	g_free (str);

	return TRUE;
}

/**
 * here the joy begins
 */
int main(int argc, char *argv[]) {
	int ret, opt;
	GIOChannel *io_stdin;

	// capture cntrl-c
	signal (SIGINT,my_signal_handler);

	/* parse commandline */
    while ((opt = getopt(argc, argv, "a:b:d:t:csrhf:")) != -1)
    {
		switch (opt) {
			case 'a':   // adapter
				adapter = strdup(optarg);
				break;

			case 'b':   // provided BLE address
				if (dev_name) {
					fprintf(stderr, "Provide EITHER peripheral devicename or BLE address. NOT both.\n");
					exit(-1);
				}
				dev_addr = strdup(optarg);
				break;

			case 'c':	// display comma separated.
				DisplayCommaSep = true;
				break;

			case 'd':	// devicename to look for
				if (dev_addr) {
					fprintf(stderr, "Provide EITHER peripheral devicename or BLE address. Not both.\n");
					exit(-1);
				}
				dev_name = strdup(optarg);
				break;

			case 'f':   // provided filename to store data received
				FileName = strdup(optarg);
				break;

			case 't':	// set scan timeout
				timeout = (int) strtod(optarg, NULL);
				if (timeout == 0) {
					fprintf(stderr, "%d invalid timeout.\n", timeout);
					exit(-1);
				}
				break;

			case 's':	// set to reduce scan results display
				SHOW_scan = 0;
				break;

			case 'r':	// remove timestampset to reduce scan results display
				TimeStamp = false;
				break;

			case 'h':	// help
				usage(argv);
				break;

			default:
				fprintf(stderr, "Unknown option %d.\n", opt);
				usage(argv);
				break;
		}
    }

	// if no BLE address
	if (! dev_addr) {
		// AND no devicename : use default devicename
		if (! dev_name) dev_name = strdup(default_dev_name);
	}

	// set keyboard handling
	io_stdin = g_io_channel_unix_new (fileno (stdin));
	g_io_add_watch (io_stdin, G_IO_IN, (GIOFunc) handle_keyboard, NULL);

	// UUID receive notifications
	if (gattlib_string_to_uuid(BME280rCharacteristic, 36 + 1, &r_uuid) < 0) {
		fprintf(stderr, "Can not translate receive characteristic.\n");
		exit(1);
	}

	// UUID send commands
	if (gattlib_string_to_uuid(BME280sCharacteristic, 36 + 1, &s_uuid) < 0) {
		fprintf(stderr, "Can not translate send characteristic.\n");
		exit(1);
	}

	// look for device by name
	ret = Find_device();
	if (ret) exit(1);

	if (! dev_discovered){
		if (dev_addr) fprintf(stderr,"Device with address '%s' was NOT discovered.\n", dev_addr);
		else fprintf(stderr,"Device with name '%s' was NOT discovered.\n", dev_name);
		exit(2);
	}

	fprintf(stderr,"Trying to connect.\n");

	connection = gattlib_connect(NULL, dev_addr, BDADDR_LE_PUBLIC, BT_SEC_LOW, 0, 0);
	if (connection == NULL) {
		if (dev_addr) fprintf(stderr,"Fail to connect to the bluetooth device %s.\n", dev_addr);
		else fprintf(stderr,"Fail to connect to the bluetooth device %s.\n",dev_name);
		exit(3);
	}

	fprintf(stderr,"Connected.\n");

	// register disconnect call back
	ret = gattlib_register_disconnect(connection, disconnect_handler);
	if (ret) {
		fprintf(stderr, "Fail to set disconnect callback.\n");
		cleanup(4);
	}

	// set notification call back
	gattlib_register_notification(connection, notification_handler, NULL);

	// enable notifications
	ret = gattlib_notification_start(connection, &r_uuid);
	if (ret) {
		fprintf(stderr, "Fail to start notification.\n");
		cleanup(5);
	}

	fprintf(stderr, "Started notification. Waiting for data.\n");

	loop = g_main_loop_new(NULL, 0);
	g_main_loop_run(loop);

	// we are done
	fprintf(stderr, "Ending program\n");
	g_main_loop_unref(loop);
	loop = NULL;
	cleanup(0);
}

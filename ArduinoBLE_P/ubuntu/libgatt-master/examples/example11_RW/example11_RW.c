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
 ****************************************************************************
 *
 * This RWexample started based on read_write.c example, but that ended
 * up to be only 10% of the RWexample.
 *
 * It has a large number of command line options to demonstrate different
 * approaches.
 *
 * This example is menu driven. It can scan based on provided BLE address or
 * BLE name. IF not provided it will use a default name, which is the same
 * as the example11_ph_RW.ino.
 *
 * Once connected you can provided interactive requests to receive Ascii string, or
 * binary data from the peripheral. You can also input ASCII data to be send or use
 * command line provided ASCII string.
 * Received binary data and ASCII string data can also be saved to different files.
 *
 * See usage() for different options
 *
 * paulvha/ November 2022 / version 1.0
 *
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
# include <ctype.h>

// keyboard operation
typedef enum {READ, WRITE, WRITEC, READB, WRITEB, WRITEBC, NONE} operation_t;
operation_t g_operation = NONE;

static uuid_t s_w_uuid, b_w_uuid;		// remote reading, so write for us
static uuid_t s_r_uuid, b_r_uuid;		// remote writing, for us read

#define CHARACTERISTIC_R_BT_UUID  "9e400002-b5a3-f393-e0a9-e50e24dcca9e" // writing for us
#define CHARACTERISTIC_R_STR_UUID "9e400003-b5a3-f393-e0a9-e50e24dcca9e" // writing for us
#define CHARACTERISTIC_W_BT_UUID  "9e400004-b5a3-f393-e0a9-e50e24dcca9e" // reading for us
#define CHARACTERISTIC_W_STR_UUID "9e400005-b5a3-f393-e0a9-e50e24dcca9e" // reading for us

// default devicename if NO BLE address or devicename provided on command line
char *default_dev_name = "Artemis EXCHANGE BLE";

gatt_connection_t* connection = NULL;
GMainLoop* loop = NULL;
void* adapter = NULL;
char* dev_addr = NULL;			// holds the peripheral address
char* dev_name = NULL;			// holds the peripheral name (if known)
char* StrFileName = NULL;		// filename to store received Ascii string data
char* BinFileName = NULL;		// filename to store received binary information
char* A_buffer = NULL;			// holds the Ascii data from the command line
uint8_t* B_buffer = NULL;		// holds translated binary data from command line
uint8_t B_outLength;			// length fo binary buffer

bool dev_discovered = false;	// true = peripheral device is detected
bool TimeStamp = true;			// true = add timestamp to output
bool DisplayCommaSep = false;	// true = display comma separated
bool SHOW_scan = true;			// true = show all scan discovered devices
int timeout = 10;				// default scantime in seconds

#define MAGICNUM 0xcf			// byte 0 /start of each message
uint8_t buffer[100];
uint8_t BinaryTestbuffer[]= {MAGICNUM, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};

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
 * usage
 */
static void usage(char *argv[]) {
	fprintf(stderr, "%s  [-a adapter] [-b address] [-d devicename] [-t scan timeout]\n", argv[0]);
	fprintf(stderr, "    [-f filename] [-g filename] [-l binary data] [-m ascii data]\n");
	fprintf(stderr, "    [-s] [-r]\n\n");
	fprintf(stderr, "-a  option to provide local BLE adapter name.\n");
	fprintf(stderr, "-b  provide the remote BLE address instead of name.\n");
	fprintf(stderr, "-c  display binary data comma separated.\n");
	fprintf(stderr, "-d  provide the remote BLE devicename instead of address.\n");
	fprintf(stderr, "-f  provide the filename to store received Ascii strings.\n");
	fprintf(stderr, "-g  provide the filename to store received binary data.\n");
	fprintf(stderr, "-l  provide the binary data to send instead of test data.\n");
	fprintf(stderr, "-m  provide the ascii data to send instead of input from keyboard.\n");
	fprintf(stderr, "-r  remove timestamp from data from file stored data.\n");
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

		// if memory for binary data was allocated
		if (B_buffer) free(B_buffer);

		fprintf(stderr,"Bye, exit code: %d\n", ret);
		exit(ret);
	}
	else // stop loop first
		g_main_loop_quit(loop);
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
 * display keyboard / interactive commands possible
 */
static void usage_keyb() {

	fprintf(stderr, "1	Read a string from peripheral\n");
	fprintf(stderr, "2	Read a Binary buffer from peripheral\n");
	fprintf(stderr, "3	Send a ASCII string to peripheral\n");
	fprintf(stderr, "4	Send a Binary Testbuffer to peripheral\n");
	fprintf(stderr, "5	Send command line provided ASCII string to peripheral\n");
	fprintf(stderr, "6	Send command line provided Binary buffer to peripheral\n");
	fprintf(stderr, "q	Quit program\n");
}

/**
 * save received data to a file
 *
 * @param fileName : the filename to use
 * @param buffer : content to write
 * @param lenBuf : length of buffer
 * @param Bin    : True : save as binary, false : save as String
 */
void SaveToFile( char *fileName, uint8_t *buffer, int lenBuf, bool Bin){
	FILE * pFile;
	char tm[30];

	// time stamp needed
	if (TimeStamp) time_stamp(tm);

	if (Bin) {

		// open file append / binary
		pFile=fopen(fileName, "ab");

		if( ! pFile ) {
			fprintf(stderr, "Error opening file: %s.", fileName);
			return;
		}

		if (TimeStamp) {
			if (DisplayCommaSep) fprintf(pFile,"%s,",tm);
			else fprintf(pFile,"%s: ",tm);
		}

		// write binary
		if (DisplayCommaSep) {
			for (int i = 0 ; i < lenBuf ; i++){
				fwrite(&buffer[i],1,1,pFile);
				fprintf(pFile,",");
			}
		}
		else
			fwrite(buffer,lenBuf, 1, pFile);

		// write binary as ASCII
		//for (int i = 0 ; i < lenBuf ; i++){
		//	if (DisplayCommaSep) fprintf(pFile,"%x,",buffer[i]);
		//	else fprintf(pFile,"%x",buffer[i]);
		//}
	}
	else {
		// open file append
		pFile=fopen(fileName, "a");

		if( ! pFile ) {
			fprintf(stderr, "Error opening file: %s.", fileName);
			return;
		}

		if (TimeStamp) fprintf(pFile,"%s: ",tm);

		fprintf(pFile,"%s",buffer);
	}

	// add EOL
	fprintf(pFile,"\n");

	// close file
	fclose(pFile);
}

/**
 * Process keyboard input
 */
static gboolean
handle_keyboard (GIOChannel * source, GIOCondition cond)
{
	gchar *str = NULL;
	int ret;
	size_t len;

	if (g_io_channel_read_line (source, &str, NULL, NULL,
	  NULL) != G_IO_STATUS_NORMAL) {
		return TRUE;
	}

	// requesting to stop
	if (str[0] == 'q' || str[0] == 'Q') {
		g_free(str);
		cleanup(0);
		return TRUE;
	}
	// if only <enter>
	else if (str[0] == 0x0a) {
		usage_keyb();
		g_free(str);
		return TRUE;
	}

	else if (g_operation == NONE){

		if (str[0] == '1')      g_operation = READ;

		else if (str[0] == '2') g_operation = READB;

		else if (str[0] == '3') {
			g_operation = WRITE;
			fprintf(stderr, "Input ASCII data to send\n");
			g_free(str);
			return TRUE;
		}

		else if (str[0] == '4') g_operation = WRITEB;

		else if (str[0] == '5') g_operation = WRITEC;

		else if (str[0] == '6') g_operation = WRITEBC;

		else {
			fprintf(stderr, "Invalid request %d\n", str[0]);
			g_free(str);
			return TRUE;
		}
	}

	// if WRITE was set before, we now should have the Ascii string to send
	if (g_operation == WRITE){
		fprintf(stderr,"Send Ascii string.\n");

		gchar *a = malloc(strlen(str)+1);

		if (! a) {
			fprintf(stderr,"could not obtain memory to send string\n");
			g_free(str);
			return TRUE;
		}

		// set magic number
		a[0] = (gchar) MAGICNUM;

		uint8_t i;
		for (i = 0 ; i < strlen(str);i++)
			a[i+1] = str[i];

		if (i != 0) {
			ret = gattlib_write_char_by_uuid(connection, &s_w_uuid, a, strlen(a));

			if (ret) {
				fprintf(stderr, "Failed to send keyboard data %s.\n", a);
			}
		}

		free(a);
	}

	// send commandline provided ASCII string
	if (g_operation == WRITEC){
		gchar *a;

		// a command line provided Ascii string
		if (! A_buffer) {
			fprintf(stderr,"NO Ascii string provided on command line.\n");
		}
		else {
			fprintf(stderr,"Send Ascii string command line.\n");

			a = malloc(strlen(A_buffer)+1);

			if (! a) {
				fprintf(stderr,"Could not obtain memory to send string.\n");
				g_free(str);
				return TRUE;
			}
			// set magic number
			a[0] = (gchar) MAGICNUM;

			uint8_t i;
			for (i = 0 ; i < strlen(A_buffer);i++)
				a[i+1] = A_buffer[i];

			if (i != 0) {
				ret = gattlib_write_char_by_uuid(connection, &s_w_uuid, a, strlen(a));

				if (ret) {
					fprintf(stderr,"Failed to send keyboard data %s.\n", a);
				}
			}
		}

		free(a);
	}

	// send a binary
	else if (g_operation == WRITEB) {
		fprintf(stderr,"Send Binary Testbuffer.\n");

		ret = gattlib_write_char_by_uuid(connection, &b_w_uuid, BinaryTestbuffer, sizeof(BinaryTestbuffer));
		assert(ret == 0);

		fprintf(stderr,"Binary write to UUID completed: ");
		// skip magicnumber
		for (int i = 1; i < sizeof(BinaryTestbuffer); i++)
			printf("0x%02x ", BinaryTestbuffer[i]);
		printf("\n");
	}

	// send a binary buffer provided on command line
	// (magicnum already added during GetBinaryInput()
	else if (g_operation == WRITEBC) {

		if (! B_buffer) {
			fprintf(stderr,"No Binary buffer provided on command line");
		}
		else {
			fprintf(stderr,"Send Binary buffer command line.\n");

			ret = gattlib_write_char_by_uuid(connection, &b_w_uuid, B_buffer, B_outLength);
			assert(ret == 0);

			printf("Binary write to UUID completed: ");
			for (int i = 1; i < B_outLength; i++)
				printf("0x%02x ", B_buffer[i]);
			printf("\n");
		}
	}
	// read
	else if (g_operation == READ) {
		fprintf(stderr,"read Ascii string\n");
		len = sizeof(buffer);
		ret = gattlib_read_char_by_uuid(connection, &s_r_uuid, buffer, &len);
		if (ret != 0)
			fprintf(stderr,"Error during trying to read string\n");
		else {

			if (buffer[0] != MAGICNUM){
				fprintf(stderr,"Invalid MagicNUm 0x%x\n", buffer[0]);
			}

			else {
				fprintf(stderr,"Read Ascii String UUID completed: ");
				// skip magic
				for (int i = 1; i < len; i++)
					printf("%c", buffer[i]);

				printf("\n");

				// Do we need to save to file ?
				if (StrFileName) SaveToFile(StrFileName, buffer+1, len-1, false);
			}
		}
	}

	// read binary
	else if (g_operation == READB) {
		fprintf(stderr,"read Binary\n");
		len = sizeof(buffer);
		ret = gattlib_read_char_by_uuid(connection, &b_r_uuid, buffer, &len);
		if (ret != 0)
			fprintf(stderr,"Error during trying to read binary\n");
		else {
			if (buffer[0] != MAGICNUM){
				fprintf(stderr,"Invalid MagicNUm 0x%x\n", buffer[0]);
			}
			else {

				fprintf(stderr,"Read Binary UUID completed: ");
				for (int i = 1; i < len; i++) {
					if (DisplayCommaSep) printf("%02x,", buffer[i]);
					else  printf("%02x ", buffer[i]);
				}
				printf("\n");

				// Do we need to save to file ?
				if (BinFileName) SaveToFile(BinFileName, buffer+1, len-1, true);
			}
		}
	}

	g_operation = NONE;

	g_free (str);

	return TRUE;
}

/**
 * transfer binary string to real binary
 * @param input : pointer to command line provided data
 * command line can be provided as
 *  -l '454647'   OR
 *  -l '0x450x460x47' OR
 *  -l '0x45 0x46 0x47' OR
 *  -l '0x45, 0x46, 0X47 OR
 *
 *  any combination of the above
 */
void GetBinaryInput(char *input){

	uint8_t lenc = strlen(input);
	uint8_t i, h, cnt, t;

	B_buffer = (uint8_t *) malloc(lenc +1);

	if (! B_buffer){
		fprintf(stderr,"could not allocate memory for binary input\n");
		exit(-1);
	}

	// each message needs to start with MAGICNUM
	B_buffer[0] = MAGICNUM;

	B_outLength = 1;

	for (i = 0, cnt = 0 ; i < lenc; i++) {

	    h = input[i];

		// something like 0x45, skip '0x' or '0X'
	    if (h == 'x' || h == 'X') {

			if (cnt != 1){
				fprintf(stderr,"Error 0x- translating %s\n", input);
			}
			cnt = 0;
			continue;
		}

		// skip spaces and comma
		if (h == ' ' || h == ',') continue;

	    // ascii to hex
	    if( h >= '0' && h <= '9') h = h - '0';
	    else if( h >= 'a' && h <= 'f') h = h - 'a' + 0xa;
	    else if( h >= 'A' && h <= 'F') h = h - 'a' + 0xA;
	    else {
			printf("Invalid binary character %c (0x%x)\n", h, h);
			free(B_buffer);
			exit(-1);
		}

	    // store in single byte
	    if (cnt == 0) {
	      t = (h & 0xf) << 4;
	      cnt=1;
	    }
	    else {
	      t |= (h & 0xf);
	      cnt=0;

	      // store in buffer
	      B_buffer[B_outLength++] = t;
	    }
	}

	if (cnt != 0) {
		fprintf(stderr,"Error during translating %s\n", input);
		free(B_buffer);
		exit(-1);
	}
}

int main(int argc, char *argv[]) {

	int ret, opt;
	GIOChannel *io_stdin;

	// capture cntrl-c
	signal (SIGINT,my_signal_handler);

	/* parse commandline */
    while ((opt = getopt(argc, argv, "a:b:d:t:csrhf:g:l:m:")) != -1)
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

			case 'f':   // provided filename to store ASCII received
				StrFileName = strdup(optarg);
				break;

			case 'g':   // provided filename to store binary received
				BinFileName = strdup(optarg);
				break;

			case 'l':	// provide binary string to send on request
				GetBinaryInput(optarg);
				break;

			case 'm':	// provide ascii string to send on request
				A_buffer = strdup(optarg);
				break;

			case 't':	// set scan timeout
				timeout = (int) strtod(optarg, NULL);
				if (timeout < 2) {
					fprintf(stderr, "%d invalid timeout.\n", timeout);
					exit(-1);
				}
				break;

			case 's':	// set to reduce scan results display
				SHOW_scan = 0;
				break;

			case 'r':	// remove timestampset to reduce file saved data
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

	// translate BINARY characteristic for us to write
	if (gattlib_string_to_uuid(CHARACTERISTIC_R_BT_UUID , strlen(CHARACTERISTIC_R_BT_UUID) + 1, &b_w_uuid) < 0) {
		usage(argv);
		return 1;
	}

	// translate STRING characteristic for us to write
	if (gattlib_string_to_uuid(CHARACTERISTIC_R_STR_UUID , strlen(CHARACTERISTIC_R_STR_UUID) + 1, &s_w_uuid) < 0) {
		usage(argv);
		return 1;
	}

	// translate BINARY characteristic for us to read
	if (gattlib_string_to_uuid(CHARACTERISTIC_W_BT_UUID , strlen(CHARACTERISTIC_W_BT_UUID) + 1, &b_r_uuid) < 0) {
		usage(argv);
		return 1;
	}

	// translate STRING characteristic for us to read
	if (gattlib_string_to_uuid(CHARACTERISTIC_W_STR_UUID , strlen(CHARACTERISTIC_W_STR_UUID) + 1, &s_r_uuid) < 0) {
		usage(argv);
		return 1;
	}

	// look for device
	ret = Find_device();
	if (ret) exit(1);

	if (! dev_discovered){
		if (dev_addr) fprintf(stderr,"Device with address '%s' was NOT discovered.\n", dev_addr);
		else fprintf(stderr,"Device with name '%s' was NOT discovered.\n", dev_name);
		exit(2);
	}

	fprintf(stderr,"Trying to connect.\n");

	connection = gattlib_connect(NULL, dev_addr, BDADDR_LE_RANDOM, BT_SEC_LOW, 0, 0);
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

	fprintf(stderr, "Ready to go.\n");

	usage_keyb();

	loop = g_main_loop_new(NULL, 0);
	g_main_loop_run(loop);

	// we are done
	fprintf(stderr, "Ending program\n");
	g_main_loop_unref(loop);
	loop = NULL;
	cleanup(0);
}

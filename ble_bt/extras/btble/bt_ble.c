/*
 *  paulvha / January 2020 / version 1.0
 *
 *  This is extreem stripped down version of gatttool and it will only try to read
 *  the battery level, set the battery resistor and read temperature in celsius
 *  or Fahrenheit from a remote device.
 *
 *  This has been created and tested to work against Apollo3 running ble_bt.ino
 *
 *
 * *******************************************************************
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2010  Nokia Corporation
 *  Copyright (C) 2010  Marcel Holtmann <marcel@holtmann.org>
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
 ******************************************************************************

 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>

#include "lib/bluetooth.h"
#include "lib/hci.h"
#include "lib/hci_lib.h"
#include "lib/sdp.h"
#include "lib/uuid.h"

#include "att.h"                                // error codes
#include "btio/btio.h"
#include "gattrib.h"
#include "gatt.h"
#include "bt_ble.h"

// DEFINE THE UUID ALWAYS IN LOWER CASE
static char batt_prim[]="180f";                 // UUID primary service for battery
static char batt_char[]="2a19";                 // UUID battery level

/* while the UUID for battery load resistor is created at Bluetooth sig,
 * it is not what Bluez is recognising. The created UUID on the server is:
 *   fc8b661f-86d5-4e33-97a4-b3fef6772838
 * nRfconn is recognizing this full UUID BUT Bluez is using the
 * last 4 digits (2838) only and then makes it
 *   00002838-0000-1000-8000-00805f9b34fb
 *
 * Hence defined as "2838"
 */
static char battload_char[]= "2838";            // UUID battery load

static char env_prim[]="181a";                  // UUID primary service for environment
static char tempC_char[]="2a1f";                // UUID characteristic for Celsius temperature
static char tempF_char[]="2a20";                // UUID characteristic for Fahrenheit temperature

static char *opt_src = NULL;                    // store local MAC
static char *opt_dst = NULL;                    // store destination MAC
static char *opt_dst_type = NULL;               // type : public ?
static char *opt_sec_level = NULL;              // security level
static int opt_mtu = 0;                         // set MTU size
static int opt_psm = 0;                         // set GATT over BD/EDR
static GMainLoop *event_loop;                   // mainloop
static gboolean got_error = FALSE;              // indicate error for exit condition
static GSourceFunc operation;                   // place to store operation function called from connect_cb
static GAttrib *b_attrib;                       // attrib

static gboolean g_debug = FALSE;                // set verbose / debug
static gboolean g_tempfahrenheit = FALSE;       // get value for temperature in Fahrenheit
static gboolean g_tempcelsius = FALSE;          // get value for Temperature in Celsius
static gboolean g_battery = FALSE;              // get value for battery / supply percentage
static gboolean g_batteryload = FALSE;          // set batteryload, read battery, remove batteryload, read battery
static gboolean b_load = FALSE;                 // is battery load resistor set or not
uint8_t GetValue = NOVALUE;                     // which value to get next

#define MAX_PRIMARY 20
struct gatt_primary r_primary[MAX_PRIMARY];     // stored received primary

static gboolean read_all_primary();


/* Check whether there is another value to lookup */
void DetermineValue()
{
    GetValue = NOVALUE;

    if (g_batteryload) {
        g_batteryload = FALSE;      // remove flag
        g_battery = TRUE;           // enable battery level reading always after battery resistor setting
        GetValue = BATTERYLOAD;
    }
    else if (g_battery)  {
        g_battery = FALSE;

        if (b_load) {               // if resistor was set we need to remove after this battery level read
            g_batteryload = TRUE;
        }
        GetValue = BATTERY;         // read the battery level
    }
    else if (g_tempcelsius) {
        g_tempcelsius = FALSE;
        GetValue = TEMPCELSIUS;
    }
    else if (g_tempfahrenheit) {
        g_tempfahrenheit = FALSE;
        GetValue = TEMPFAHRENHEIT;
    }
}

/* call back after connect has been completed */
static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
    uint16_t mtu;
    uint16_t cid;
    GError *gerr = NULL;

    if (g_debug) g_print("%s\n", __func__);

    if (err) {
        g_printerr("%s\n", err->message);
        got_error = TRUE;
        g_main_loop_quit(event_loop);
    }

    // negotiate the MTU size
    bt_io_get(io, &gerr, BT_IO_OPT_IMTU, &mtu,
                BT_IO_OPT_CID, &cid, BT_IO_OPT_INVALID);

    if (gerr) {
        g_printerr("Can't detect MTU, using default: %s",
                                gerr->message);
        g_error_free(gerr);
        mtu = ATT_DEFAULT_LE_MTU;
    }

    if (cid == ATT_CID)
        mtu = ATT_DEFAULT_LE_MTU;

    b_attrib = g_attrib_new(io, mtu, false);

    // call the first function after connect
    operation(b_attrib);
}

/* call back after reading value handle */
static void read_value(guint8 status, const guint8 *pdu, guint16 plen,
                            gpointer user_data)
{
    uint8_t value[plen];
    ssize_t vlen;
    int16_t val;

    if (g_debug) g_print("%s\n", __func__);

    if (status != 0) {
        g_printerr("Can not read value: %s\n",att_ecode2str(status));
        got_error = TRUE;
        goto done;
    }

    vlen = dec_read_resp(pdu, plen, value, sizeof(value));

    if (vlen < 0) {
        g_printerr("Protocol error\n");
        got_error = TRUE;
        goto done;
    }

    if (GetValue == BATTERY)
        g_print("Battery / supply percentage: %d%%\n", value[0]);

    else if (GetValue == TEMPCELSIUS) {
        val = value[1]<<8 | value [0];
        g_print("Temperature %2.1f C\n", (float)val/10);
    }
    else if (GetValue == TEMPFAHRENHEIT) {
        val = value[1]<<8 | value [0];
        g_print("Temperature %2.1f F\n", (float)val/10);
    }
    else {
        g_printerr("Unknown value\n");
        got_error = TRUE;
    }

done:
    g_main_loop_quit(event_loop);
}

// call back after setting battery load resistor on or off
static void char_write_req_cb(guint8 status, const guint8 *pdu, guint16 plen,
                            gpointer user_data)
{
    if (g_debug) g_print("%s\n", __func__);

    if (status) {
        g_printerr("Setting batterload failed: %s\n",
                            att_ecode2str(status));
        got_error = TRUE;
        goto done;
    }

    // Indicate whether load resistor is set or not
    b_load =! b_load;

    if (b_load) g_print("Battery resistor has been set\n");
    else  g_print("Battery resistor has been removed\n");

done:
   g_main_loop_quit(event_loop);
}

/* call back as services handles are extended with characteristic information
 * now lookup the wanted characteristic, obtain the specific handle to read the value
 */
static void read_characteristics(uint8_t status, GSList *ranges, void *user_data)
{
    GSList *l;
    char *p;
    uint8_t value;

   if (g_debug) g_print("%s\n", __func__);

    if (status) {
        g_printerr("Discover characteristics failed: %s\n",
                            att_ecode2str(status));
        got_error = TRUE;
        goto error;
    }

    // determine value characteristic UUID to look for
    if (GetValue == BATTERY) p = batt_char;
    else if (GetValue == BATTERYLOAD) p = battload_char;
    else if (GetValue == TEMPCELSIUS) p = tempC_char;
    else p = tempF_char;

    // try to find a match
    for (l = ranges; l; l = l->next) {
        struct gatt_desc *range = l->data;

        // If UUID for value
        if (strstr(range->uuid,p) != NULL) {

            if (g_debug)  g_print("FOUND: handle = 0x%04x, uuid = %s\n", range->handle, range->uuid);

            // NOW this is NOT reading.. but setting or removing batteryload resistor
            if (GetValue == BATTERYLOAD) {

                if (g_debug) g_print("updating battery load resistor\n");

                // if resistor load was set, remove it, else set
                if (b_load) value = 0;
                else value = 1;

                // call back 'char_write_req_cb' when done
                gatt_write_char(b_attrib, range->handle, &value, 1, char_write_req_cb, NULL);

                return;
            }

            // NOW this is reading .... use the handle get value
            gatt_read_char(b_attrib, range->handle, (GAttribResultFunc) read_value, b_attrib);
            return;
        }
    }

    g_printerr("Did not find the %s value UUID\n", p);
    got_error = TRUE;

error:
    g_main_loop_quit(event_loop);
}

/* lookup the wanted service UUID and it's handles
 * then read the handles to obtain each characteristic info / UUID
 */
static gboolean read_handles()
{
    int i;
    char s_uuid[132];

    if (g_debug) g_print("%s\n", __func__);

    // determine service to find
    if (GetValue == BATTERY || GetValue == BATTERYLOAD)
        sprintf(s_uuid, "0000%s-0000-1000-8000-00805f9b34fb",batt_prim);

    else if (GetValue == TEMPCELSIUS  || GetValue == TEMPFAHRENHEIT)
        sprintf(s_uuid, "0000%s-0000-1000-8000-00805f9b34fb",env_prim);

    if (g_debug) g_print("Search for uuid %s\n", s_uuid);

    // try to find service in local list
    for (i=0; i < MAX_PRIMARY; i++) {

        // lookup UUID
        if (strcmp (r_primary[i].uuid, s_uuid) == 0)
        {
            // Get handles for service : call back read_characteristics when done
            gatt_discover_desc(b_attrib,r_primary[i].range.start, r_primary[i].range.end, NULL, read_characteristics, NULL);

            if (g_debug)  g_print("FOUND start handle = 0x%04x, end handle = 0x%04x "
            "uuid: %s\n", r_primary[i].range.start, r_primary[i].range.end, r_primary[i].uuid);

            return(TRUE);
        }

        if (r_primary[i].range.end == 0x0) break;
     }

    // oh oh not good !!
    if (GetValue == BATTERY || GetValue == BATTERYLOAD)
            g_printerr("No battery service on this device\n");
    else if (GetValue == TEMPCELSIUS  || GetValue == TEMPFAHRENHEIT)
            g_printerr("No Environmental service on this device\n");
    else
            g_printerr("Service not found on this device\n");

    got_error = TRUE;

done:
    g_main_loop_quit(event_loop);
}

/* First obtain all primary services and handles
 *
 * Multiple values / characteristiscs can be requested
 * but once connnected the device will stop advertising services
 * so we store all available primary services info in the first connect
 */
static void extract_all_primary(uint8_t status, GSList *services, void *user_data)
{
    GSList *l;
    uint8_t i;

    if (g_debug) g_print("%s\n", __func__);

    if (status) {
        g_printerr("Discover all primary services failed: %s\n",
                            att_ecode2str(status));
        got_error = TRUE;
        goto done;
    }

    // copy primary services list to local structure
    for (l = services, i=0; l && i < MAX_PRIMARY; l = l->next, i++) {
        struct gatt_primary *prim = l->data;
        r_primary[i].range.start = prim->range.start;
        r_primary[i].range.end = prim->range.end;
        strcpy(r_primary[i].uuid,prim->uuid);

         if (g_debug){
            g_print("attr handle = 0x%04x, end grp handle = 0x%04x "
            "uuid: %s\n", prim->range.start, prim->range.end, prim->uuid);
        }
    }

    if (i == MAX_PRIMARY) {
        g_printerr("Exceeded maximum primary amount of %d:\n",MAX_PRIMARY);
        got_error = TRUE;
        goto done;
    }
    else     // terminate list
        r_primary[i].range.end = 0x0;

    // obtain all the handles for a specific service
    read_handles();

    return;
done:
    g_main_loop_quit(event_loop);
}

/* First obtain the all primary services and handles
 *
 * Multiple values / characteristiscs can be requested
 * but once connnected the device will stop advertising services
 * so we request them all up front after the first connect
 */
static gboolean read_all_primary()
{
    if (g_debug) g_print("%s\n", __func__);

    // set call back for extract_all_primary
    gatt_discover_primary(b_attrib, NULL, extract_all_primary, NULL);

    return(TRUE);
}

static GOptionEntry options[] = {
    { "adapter", 'i', 0, G_OPTION_ARG_STRING, &opt_src,
        "Specify local adapter interface", "hciX" },
    { "device", 'b', 0, G_OPTION_ARG_STRING, &opt_dst,
        "Specify remote Bluetooth address", "MAC" },
    { "addr-type", 't', 0, G_OPTION_ARG_STRING, &opt_dst_type,
        "Set LE address type. Default: public", "[public | random]"},
    { "mtu", 'm', 0, G_OPTION_ARG_INT, &opt_mtu,
        "Specify the MTU size", "MTU" },
    { "psm", 'p', 0, G_OPTION_ARG_INT, &opt_psm,
        "Specify the PSM for GATT/ATT over BR/EDR", "PSM" },
    { "sec-level", 'l', 0, G_OPTION_ARG_STRING, &opt_sec_level,
        "Set security level. Default: low", "[low | medium | high]"},
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &g_debug,
        "Set verbose Default: off", NULL},
    { "battery", 's', 0, G_OPTION_ARG_NONE, &g_battery,
        "Get Battery /supply precentage", NULL},
    { "celsius", 'c', 0, G_OPTION_ARG_NONE, &g_tempcelsius,
        "Get Temperature Celsius", NULL},
    { "fahrenheit", 'f', 0, G_OPTION_ARG_NONE, &g_tempfahrenheit,
        "Get Temperature Fahrenheit", NULL},
    { "resistor", 'r', 0, G_OPTION_ARG_NONE, &g_batteryload,
        "Set load resistor and measure battery", NULL},
    { NULL },
};

int main(int argc, char *argv[])
{
    GOptionContext *context;
    GError *gerr = NULL;
    GIOChannel *chan;

    // default values
    opt_dst_type = g_strdup("public");
    opt_sec_level = g_strdup("low");

    // add options and parse command line
    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, options, NULL);

    if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
        g_printerr("%s\n", gerr->message);
        g_clear_error(&gerr);
    }

    // determine value to read (first)
    DetermineValue();

    if (GetValue == NOVALUE) {
        g_printerr("No value to read was requested\n");
        got_error = TRUE;
        goto done;
    }

    if (g_debug) g_print("Trying to Connect\n");

    if (opt_dst == NULL) {
        g_printerr("Remote Bluetooth address required\n");
        got_error = TRUE;
        goto done;
    }

    chan = gatt_connect(opt_src, opt_dst, opt_dst_type, opt_sec_level,
                    opt_psm, opt_mtu, connect_cb, &gerr);

    if (chan == NULL) {
        g_printerr("%s\n", gerr->message);
        g_clear_error(&gerr);
        got_error = TRUE;
        goto done;
    }

    g_print("Starting mainloop (waiting on connect)\n");

    // set operation to perform : called at end of connect_cb
    operation = read_all_primary;

    // NULL = the default context will be used   FALSE = Not running
    event_loop = g_main_loop_new(NULL, FALSE);

    // first connect and get all primary services
    // next requested values (if any) will call read_handles to use earlier retrieved data
    bool FirstConnect = TRUE;
    while (GetValue != NOVALUE) {

        if (FirstConnect) FirstConnect = FALSE;
        else read_handles();

        g_main_loop_run(event_loop);

        // more values requested ?
        DetermineValue();
    }

done:
    if (event_loop != NULL) g_main_loop_unref(event_loop);
    g_option_context_free(context);
    g_free(opt_src);
    g_free(opt_dst);
    g_free(opt_sec_level);

    if (got_error)
        exit(EXIT_FAILURE);
    else
        exit(EXIT_SUCCESS);
}

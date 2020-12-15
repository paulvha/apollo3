/**
 *  paulvha / February 2020 / version 1.0
 *
 * This is contains parts of gatttool. It will allow
 * requesting and receiving data from an Artemis / Apollo3 based server/
 *
 * This has been created and tested to work against Apollo3 running ble_amdts.ino
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
#include "amdtc.h"
#include "src/shared/util.h"
#include "amdtc_UI.h"

/**
 * AMDTP defined information is svc_amdtp
 */
/* UUIDs */
static const uint8_t svcRxUuid[] = "00002760-08c2-11e1-9073-0e8ac72e0011";
static const uint8_t svcTxUuid[] = "00002760-08c2-11e1-9073-0e8ac72e0012";
static const uint8_t svcAckUuid[] = "00002760-08c2-11e1-9073-0e8ac72e0013";
static const uint8_t amdtpSvcUuid[] = "00002760-08c2-11e1-9073-0e8ac72e1011";

static char *opt_src = NULL;                    // store local MAC
static char *opt_dst = NULL;                    // store destination MAC
static char *opt_dst_type = NULL;               // type : public ?
static char *opt_sec_level = NULL;              // security level
static int opt_mtu = 0;                         // set MTU size
static int opt_psm = 0;                         // set GATT over BD/EDR
int opt_adc_ch = 0;                             // Adc channel to read;
int opt_pin_rd = 0;                             // Pin to read;
int opt_high_pin = 0;                           // Pin to set high;
int opt_low_pin = 0;                            // Pin to set low;
// command line commands
// make sure to add this in top of amdtc_UI.c as well
gboolean opt_TempC = FALSE;
gboolean opt_TempF = FALSE;
gboolean opt_Batt = FALSE;
gboolean opt_Batt_load = FALSE;
gboolean opt_TurnOn = FALSE;
gboolean opt_TurnOff = FALSE;
gboolean opt_interactive = FALSE;
gboolean opt_bme280 = FALSE;
gboolean opt_adc = FALSE;
gboolean opt_read_pin = FALSE;
gboolean opt_pin_high = FALSE;
gboolean opt_pin_low = FALSE;
gboolean opt_quiet = FALSE;

extern uint8_t GetValue;                 // which value to get next (defined in amdtc_UI.c)

GMainLoop *event_loop;                   // mainloop Qlib
gboolean got_error = FALSE;              // indicate error for exit condition
static GSourceFunc operation;            // place to store operation function called from connect_cb
GAttrib *b_attrib;                       // attrib
static amdtpCb_t amdtpCb;                // the AMD transfer protocol control block

#define MAX_PRIMARY 20                    // store all received primary
struct gatt_primary r_primary[MAX_PRIMARY];

uint8_t g_debug = 0;                     // set verbose / debug
uint8_t ReceivedBuf[200];                // decode
uint16_t ReceivedLen;

/*
 * As we receive bytes on a String characteristic, we can not have a zero in the middle of the
 * bytes to be received. At the sender side we translate a zero to 0x73 0x20. This routine
 * will just do the opposite and translate 0x7e 0x20 back to 0x0.
 *
 * Hope and pray that 0x7e 0x20 strings do not happen ... else we extend the decode one morestep.
 */
void encode_receipt(const uint8_t *pdu, uint16_t len)
{
  char save = 0x0;
  ReceivedLen = 0;

  for (size_t i = 0; i < len; i++) {

    char c = pdu[i];

    // Header character for 0x0 ??
    if (c == 0x7E && save == 0x0) {
        save = 0x7E;
        continue;
    }

    // did we get a header character before
    if (save == 0x7E){

      // if next is 0x20 then the real character was 0x0;
      if (c == 0x20)  c = 0x0;

      // the previous 0x7E was not header, add
      else {
        ReceivedBuf[ReceivedLen++] = save;
      }

    save = 0x0; // reset header
    }

    // Store real current character
    ReceivedBuf[ReceivedLen++] = c;
  }
}

/**
 * @brief call back after receiving notification, Either Data or acknowledgement
 */
static void parseNotification(uint16_t handle, const uint8_t *pdu, uint16_t len, gpointer user_data)
{
    uint8_t packet[len];
    eAmdtpStatus_t res;
    uint16_t i;


    if (g_debug > 1) g_print("%s\n", __func__);

    // see description
    encode_receipt(pdu, len);

    if (g_debug > 0) {
        g_print("Data received : ");
        for (i = 0; i < ReceivedLen; i++)  g_print("0x%X ", ReceivedBuf[i]);
        g_print("\n");
    }

    // copy as PDU is "constant" and AmdtpReceivePkt is not expecting that
    for (i = 0 ; i < ReceivedLen; i++) packet[i] = ReceivedBuf[i];

    if (handle == amdtpCb.ACK_handle)
        res = AmdtpReceivePkt(&amdtpCb, &amdtpCb.ackPkt, ReceivedLen, packet);
    else
        res = AmdtpReceivePkt(&amdtpCb, &amdtpCb.rxPkt, ReceivedLen, packet);

    switch(res) {
        case AMDTP_STATUS_INVALID_PKT_LENGTH:
        case AMDTP_STATUS_INSUFFICIENT_BUFFER:
        case AMDTP_STATUS_CRC_ERROR:
            g_printerr("AMDTP Protocol error\n");
            got_error = TRUE;
            goto done;
            break;

        case AMDTP_STATUS_RECEIVE_CONTINUE:
            if (g_debug > 0) g_print("continue receiving\n");
            return; // TX is done with notification.
            break;

        case AMDTP_STATUS_RECEIVE_DONE:
            if (g_debug > 0) g_print("complete packet received\n");

            // now interprete the complete package
            // from here MainLoop call will be done for the
            // data received for user interface to handle

            AmdtpPacketHandler(&amdtpCb);

            return;
            break;

        default:
            g_printerr("AMDTP unknown error\n");
            got_error = TRUE;
            goto done;
            break;
    }

done:
    g_main_loop_quit(event_loop);
}

/**
 *  @brief notifications handler
 *  called when data or acknowledgement was received from server as notification
 *  is called from gattrib.c / attrib_callback_notify()
 */
static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
    GAttrib *attrib = user_data;
    uint8_t *opdu;
    uint16_t handle, i, olen = 0;
    size_t plen;

    if (g_debug > 1) g_print("%s\n", __func__);

    handle = get_le16(&pdu[1]);

    switch (pdu[0]) {

        case ATT_OP_HANDLE_NOTIFY:

            if (g_debug > 1) {
                g_print("Notification handle = 0x%04x ", handle);
                if (handle == amdtpCb.RX_handle ) g_print("(data)\n");
                else g_print("(Acknowledge)\n");

            }
            // +3  = skip notify or indication indicator and handle
            parseNotification(handle,  &pdu[3], len - 3, user_data);

            return;

            break;

        case ATT_OP_HANDLE_IND: // info only.. not used at this time . Future ???
            g_print("Indication on handle = 0x%04x value:  )", handle);
            break;

        default:
            g_print("Invalid event\n");
            return;
    }

    // sent something back on indication request
    for (i = 3; i < len; i++)  g_print("%02x ", pdu[i]);
    g_print(" Ignored\n");

    opdu = g_attrib_get_buffer(attrib, &plen);
    olen = enc_confirmation(opdu, plen);

    if (olen > 0)
        g_attrib_send(attrib, 0, opdu, olen, NULL, NULL, NULL);
}

/**
 * @brief start notification & indication listeners
 */
static gboolean listen_start(gpointer user_data)
{
    if (g_debug > 1) g_print("%s\n", __func__);

    g_attrib_register(b_attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES,
                        events_handler, b_attrib, NULL);
    g_attrib_register(b_attrib, ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES,
                        events_handler, b_attrib, NULL);

    // if the function returns FALSE it is automatically removed from the list of event sources and will not be called again.
    return FALSE;
}

/**
 * @brief call back after connect has been completed
 *
 * This is the start of the glib loop
 */
static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
    uint16_t mtu;
    uint16_t cid;
    GError *gerr = NULL;


    if (g_debug > 1) g_print("%s\n", __func__);

    if (err) {
        g_printerr("%s\n", err->message);
        got_error = TRUE;
        g_main_loop_quit(event_loop);
        return;
    }

    if (! opt_quiet) g_print("Connected\n");

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

    // set maximum MTU size for AMDTP
    amdtpCb.attMtuSize = mtu;

    // indicate connected for AMDTP
    amdtpCb.txState = AMDTP_STATE_TX_IDLE;
    amdtpCb.txReady = true;

    // start notification listeners
    g_idle_add(listen_start,b_attrib);

    // call the first function after connect (defined in main())
    operation(b_attrib);
}

/**
 *  @brief call back from writing a command / request
 *
 * callback from characteristics_write_req()
 */
static void char_write_req_cb(guint8 status, const guint8 *pdu, guint16 plen,
                            gpointer user_data)
{
    if (g_debug > 1) g_print("%s\n", __func__);

    if (status != 0) {
        g_print("Bluetooth error\n");
       // g_printerr("Command Write Request failed: %s\n", att_ecode2str(status));
        goto done;
    }

    if (! dec_write_resp(pdu, plen) && !dec_exec_write_resp(pdu, plen)) {
        g_printerr("Bluetooth Protocol error\n");
        goto done;
    }

    // if complete packet was not sent yet, continue sending
    if (amdtpCb.txState == AMDTP_STATE_SENDING) {

        if (g_debug > 0)
            g_print("request was written successfully. continue sending \n");

            AmdtpSendPacketHandler(&amdtpCb,(GAttribResultFunc) char_write_req_cb);
            return;
    }

    if (g_debug > 0)
        g_print("Request was written successfully. Waiting for respond notification\n");

    return;

done:
    got_error = TRUE;
    g_main_loop_quit(event_loop);
}

/**
 * @brief write a command / request to the server
 */
gboolean characteristics_write_req(uint8_t cmd, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    if (g_debug > 1) g_print("%s\n", __func__);

    char sbuf[MAX_BUF];
    sbuf[0] = cmd;

    if (len > 0){
        if (len > MAX_BUF - 1) return FALSE;
        for (i = 0; i < len; i++) sbuf[i+1] = buf[i];
    }

    if (amdtpCb.TX_handle <= 0) {
        g_printerr("A valid transmit handle is required\n");
        return FALSE;
    }

    /* Build packet
     *
     * amdtpCb   : control block
     * type      : data, ack or control
     * encrypted : encryption or not    (FALSE)
     * enableAck : does not seem to be used (FALSE)
     * buf       : request to sent
     * len       : length of request to sent
     */

    AmdtpBuildPkt(&amdtpCb, AMDTP_PKT_TYPE_DATA, FALSE, FALSE, sbuf, len+1);

    // sent packet and call back char_write_req_cb
    AmdtpSendPacketHandler(&amdtpCb, (GAttribResultFunc) char_write_req_cb);

    return TRUE;
}

/**
 *  @brief call back from enabling notifications.
 */
static void enable_comms_cb(guint8 status, const guint8 *pdu, guint16 plen,
                            gpointer user_data)
{
    if (g_debug > 1) g_print("%s\n", __func__);

    if (status != 0) {
        g_printerr("Notification Write Request failed: %s\n", att_ecode2str(status));
        goto error;
    }

    // now sent hallo to server
    if (characteristics_write_req(AMDTP_CMD_HELLO,NULL,0))  return;

error:
    got_error = TRUE;
    g_main_loop_quit(event_loop);
}

/**
 * @brief write the CCC to enable transmitting ACK and data notificatons
 */
static void enable_comms()
{

    if (g_debug > 1) g_print("%s\n", __func__);

    uint16_t handle;
    uint8_t value[2];

    value[0]=0x01;      // enable notification
    value[1]=0x00;

    // enable TX notify (CCCD)
    handle = amdtpCb.RX_handle + 1;

    gatt_write_char(b_attrib, handle, value,2, enable_comms_cb, NULL);

    // enable ACK notify (CCCD)
    handle = amdtpCb.ACK_handle + 1;

    gatt_write_char(b_attrib, handle, value, 2, enable_comms_cb, NULL);
}

/**
 * @brief call back as service handles are extended with characteristic information
 *
 * callback from read_handles()
 *
 * For AMDTP the handles are already defined in the svc_amdtp.h / amdtc.h
 * we will only validate here whether all handles have been found. We then assume
 * we are connected to the same service
 *
 */
static void read_characteristics(uint8_t status, GSList *ranges, void *user_data)
{
    GSList *l;
    uint8_t t; //count matched handles

   if (g_debug > 1) g_print("%s\n", __func__);

    if (status) {
        g_print("error\n");
       // g_printerr("Discover characteristics failed: %s\n", att_ecode2str(status));
        goto error;
    }

    t = 0;
    /* so we now have all the handles for the amdtp-service
     * and validate we are in sync on the handles */

    for (l = ranges; l; l = l->next) {
        struct gatt_char *range = l->data;

        // RX UUID found (RX for Artemis, TX for us)
        if (strcmp (range->uuid, svcRxUuid) == 0) {
            amdtpCb.TX_handle = range->value_handle;
            t++;
        }
        // TX UUID (TX for Artemis, RX for us)
        else if (strcmp (range->uuid, svcTxUuid) == 0){
            amdtpCb.RX_handle = range->value_handle;
            t++;
        }
        // ACK UUID
        else if (strcmp (range->uuid, svcAckUuid) == 0){
            amdtpCb.ACK_handle = range->value_handle;
            t++;
        }

        if (g_debug > 0)  g_print("Value handle = 0x%04x, uuid = %s, count = %d of 3\n", range->value_handle, range->uuid, t);
    }

    // check we have a good match
    if (t != 3) {
        g_printerr("Could not get all the handles\n. Service mismatch !!");
        goto error;
    }

    /** OK, so we are connected, we have found the service and validated the handles */
    // enable notification communication on the server
    enable_comms();

    //
   // g_idle_add(read_interactive, b_attrib);

    return;

error:
    got_error = TRUE;
    g_main_loop_quit(event_loop);
}

/**
 * @brief lookup the wanted service UUID and it's handles
 *
 * called from extract_all_primary()
 *
 * then read the start & end handles to obtain each characteristic info / UUID
 */
static gboolean read_handles()
{
    int i;

    if (g_debug > 1) g_print("%s\n", __func__);

    // try to find service in local list
    for (i = 0; i < MAX_PRIMARY; i++) {

        // lookup AMDTP UUID
        if (strcmp (r_primary[i].uuid, amdtpSvcUuid) == 0)
        {
            // Get handles for service : call back read_characteristics when done
            gatt_discover_char(b_attrib, r_primary[i].range.start, r_primary[i].range.end, NULL, read_characteristics, NULL);

            if (opt_quiet == FALSE) g_print("FOUND AMDTP service\n\tstart handle = 0x%04x\n\tend handle = 0x%04x\n\tuuid: %s\n", r_primary[i].range.start, r_primary[i].range.end, r_primary[i].uuid);

            return(TRUE);
        }

        if (r_primary[i].range.end == 0x0) break;
     }

    // oh oh not good !!
    g_printerr("AMDTP Service not found on this device\n");

    got_error = TRUE;

done:
    g_main_loop_quit(event_loop);
}

/**
 * @brief First obtain all primary services and handles
 *
 * call back from read_all_primary()
 *
 * Multiple values / characteristiscs can be requested
 * but once connnected the device will stop advertising services
 * so we store all available primary services info in the first connect
 */
static void extract_all_primary(uint8_t status, GSList *services, void *user_data)
{
    GSList *l;
    uint8_t i;

    if (g_debug > 1) g_print("%s\n", __func__);

    if (status) {
        g_printerr("Discover all primary services failed: %s\n", att_ecode2str(status));
        got_error = TRUE;
        goto done;
    }

    // copy primary services list to local structure
    for (l = services, i = 0; l && i < MAX_PRIMARY; l = l->next, i++) {
        struct gatt_primary *prim = l->data;
        r_primary[i].range.start = prim->range.start;
        r_primary[i].range.end = prim->range.end;
        strcpy(r_primary[i].uuid,prim->uuid);

        if (g_debug > 0){
            g_print("attr handle = 0x%04x, end grp handle = 0x%04x "
            "uuid: %s\n", prim->range.start, prim->range.end, prim->uuid);
        }
    }

    if (i == MAX_PRIMARY) {
        g_printerr("Exceeded maximum primary amount of %d.\n",MAX_PRIMARY);
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

/**
 * @brief First obtain the all primary services and handles
 *
 * called from connect_cb()
 * Multiple values / characteristiscs can be requested
 * but once connnected the device will stop advertising services
 * so we request them all up front after the first connect
 */
static gboolean read_all_primary()
{
    if (g_debug > 1) g_print("%s\n", __func__);

    // set call back for extract_all_primary
    gatt_discover_primary(b_attrib, NULL, extract_all_primary, NULL);

    return(TRUE);
}

/**
 * @brief define the user interface command options
 *
 * NOTE
 * For each command line option copy /past an entry
 *
 * The option (like  opt_TempC)must have been defined above
 * in this program
 */
static GOptionEntry cmd_options[] = {
    { "tempC", 0, 0, G_OPTION_ARG_NONE, &opt_TempC,
        "Read temperature in Celsius", NULL },
    { "tempF", 0, 0, G_OPTION_ARG_NONE, &opt_TempF,
        "Read temperature in Fahrenheit", NULL },
    { "battery", 0, 0, G_OPTION_ARG_NONE, &opt_Batt,
        "Read Battery percentage", NULL },
    { "battery with load", 0, 0, G_OPTION_ARG_NONE, &opt_Batt_load,
        "Read Battery percentage with load resistor", NULL },
    { "turnOn", 0, 0, G_OPTION_ARG_NONE, &opt_TurnOn,
        "Turn LED on", NULL },
    { "turnOff", 0, 0, G_OPTION_ARG_NONE, &opt_TurnOff,
        "Turn LED off", NULL },
    { "bme280", 0, 0, G_OPTION_ARG_NONE, &opt_bme280,
        "read BME280", NULL },
    { "adc", 'A', 0, G_OPTION_ARG_INT, &opt_adc_ch,
        "read adc channel", NULL },
    { "read", 'R', 0, G_OPTION_ARG_INT, &opt_pin_rd,
        "read digital pin", NULL },
    { "high", 'H', 0, G_OPTION_ARG_INT, &opt_pin_high,
        "set digital pin high", NULL },
    { "low", 'L', 0, G_OPTION_ARG_INT, &opt_pin_low,
        "set digital pin low", NULL },
    { "interactive", 'I', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
        &opt_interactive, "Use interactive mode", NULL },
    { "quiet", 'q', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
        &opt_quiet, "ONLY display result", NULL },
    { "verbose", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT, &g_debug,
        "Set verbose 0 = off, 1 = data only, 2 = all", NULL},
    { NULL },
};

/**
 * @brief define Bluetooth command line options
 */
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
    { NULL },
};

int main(int argc, char *argv[])
{
    GOptionContext *context;
    GError *gerr = NULL;
    GIOChannel *chan;
    GOptionGroup *cmd_group;

    // default values
    opt_dst_type = g_strdup("public");
    opt_sec_level = g_strdup("low");

    // add Bluetooth options
    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, options, NULL);

    /* add user interface commands */
    cmd_group = g_option_group_new("cmd", "server commands",
                    "Show all server commands", NULL, NULL);
    g_option_context_add_group(context, cmd_group);
    g_option_group_add_entries(cmd_group, cmd_options);

    // now parse the command line
    if (!g_option_context_parse(context, &argc, &argv, &gerr)) {
        g_printerr("%s\n", gerr->message);
        g_clear_error(&gerr);
        got_error = TRUE;
        goto done;
    }

    // check for conflicting request
    if (opt_quiet && g_debug != 0) {
        g_print("you can not request quiet AND verbose messages\n");
        got_error = TRUE;
        goto done;
    }

    // check for ADC channel
    if (opt_adc_ch) {
        opt_adc = TRUE;
    }

    // validate read pin
    if (opt_pin_rd) {

        if (validate_pin(opt_pin_rd))
            opt_read_pin = TRUE;
        else
            opt_pin_rd = 0;
    }

    // validate set pin high
    if (opt_pin_high) {

        if (validate_pin(opt_pin_high))
            opt_high_pin = TRUE;
        else
            opt_pin_high = 0;
    }

    // validate set pin low
    if (opt_pin_low) {

        if (validate_pin(opt_pin_low))
            opt_low_pin = TRUE;
        else
            opt_pin_low = 0;
    }

    // check whether any user interface commands were selected
    DetermineValue(FALSE);

    // check something to do
    if (! opt_interactive && GetValue == AMDTP_CMD_NONE) {
        g_printerr(" NO display selection\n");
        got_error = TRUE;
        goto done;
    };

    // initialise the structures
    if (amdtc_init(&amdtpCb) == FALSE) {
        g_printerr("Error during init\n");
        got_error = TRUE;
        goto done;
    }

    if (g_debug > 1) g_print("Trying to Connect\n");

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

    // set operation to perform : called at end of connect_cb()
    operation = read_all_primary;

    // NULL = the default context will be used   FALSE = Not running
    event_loop = g_main_loop_new(NULL, FALSE);

    if (opt_quiet == FALSE) g_print("Starting mainloop (waiting on connect)\n");
    g_main_loop_run(event_loop);

done:
    rst_terminal();  // reset from canonical
    if (event_loop != NULL) g_main_loop_unref(event_loop);
    g_option_context_free(context);
    g_free(opt_src);
    g_free(opt_dst);
    g_free(opt_sec_level);

    // allocated during init
    g_free(amdtpCb.rxPkt.data);
    g_free(amdtpCb.txPkt.data);
    g_free(amdtpCb.ackPkt.data);

    if (got_error) exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
}

/**
 * paulvha / February 2020 / version 1.0
 *
 * This is the header file for the amdtc client software.
 * It will allow requesting and receiving data from an Artemis / Apollo3 based server/
 *
 * This has been created and tested to work against Apollo3 running ble_amdts.ino
 *
 * Parts are coming from the Bluez stack and parts from the amdts / amdtc service
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

#include "amdtc/amdtpcommon/amdtp_common.h"
#include "amdtc_UI.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// foreward declarations

GAttrib *b_attrib;
gboolean got_error;
GMainLoop *event_loop;

GIOChannel *gatt_connect(const char *src, const char *dst,
            const char *dst_type, const char *sec_level,
            int psm, int mtu, BtIOConnect connect_cb,
            GError **gerr);

size_t gatt_attr_data_from_string(const char *str, uint8_t **data);
gboolean characteristics_write_req(uint8_t cmd, uint8_t *buf, uint8_t len);
static gboolean read_all_primary();

/**
 * handles for AM DTP Service.
 *
 * This was originally defined in the _svc_amdtp.h
 * Copyright (c) 2019, Ambiq Micro / All rights reserved.
 */
#define AMDTPS_START_HDL               0x0800  //0x300

enum
{
  AMDTP_SVC_HDL = AMDTPS_START_HDL,     /* AMDTP service declaration */
  AMDTPS_RX_CH_HDL,                     /* AMDTP write command characteristic */
  AMDTPS_RX_HDL,                        /* AMDTP write command data */
  AMDTPS_TX_CH_HDL,                     /* AMDTP notify characteristic */
  AMDTPS_TX_HDL,                        /* AMDTP notify data */
  AMDTPS_TX_CH_CCC_HDL,                 /* AMDTP notify client characteristic configuration */
  AMDTPS_ACK_CH_HDL,                    /* AMDTP rx ack characteristic */
  AMDTPS_ACK_HDL,                       /* AMDTP rx ack data */
  AMDTPS_ACK_CH_CCC_HDL,                /* AMDTP rx ack client characteristic configuration */
  AMDTPS_MAX_HDL
};

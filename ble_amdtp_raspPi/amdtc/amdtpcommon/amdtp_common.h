// ****************************************************************************
//
//  amdtp_common.h
//! @file
//!
//! @brief This file provides the shared functions for the AMDTP service.
//!
//! @{
//
// ****************************************************************************
/**
 * paulvha / February 2020 / version 1.0
 *
 * Based on the original amdtp_common.h, this has been changed and extended
 * to work with the Bluez Bluetooth stack on linux
 *
 * This has been created and tested to work against Apollo3-board running ble_amdts.ino
 */
//*****************************************************************************
//
// Copyright (c) 2019, Ambiq Micro
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision v2.2.0-7-g63f7c2ba1 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#ifndef AMDTP_COMMON_H
#define AMDTP_COMMON_H


#include <glib.h>

#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Macro definitions
//
//*****************************************************************************
#define AMDTP_MAX_PAYLOAD_SIZE          512
#define AMDTP_PACKET_SIZE               (AMDTP_MAX_PAYLOAD_SIZE + AMDTP_PREFIX_SIZE_IN_PKT + AMDTP_CRC_SIZE_IN_PKT)    // Bytes
#define AMDTP_LENGTH_SIZE_IN_PKT        2
#define AMDTP_HEADER_SIZE_IN_PKT        2
#define AMDTP_CRC_SIZE_IN_PKT           4
#define AMDTP_PREFIX_SIZE_IN_PKT        AMDTP_LENGTH_SIZE_IN_PKT + AMDTP_HEADER_SIZE_IN_PKT

#define PACKET_TYPE_BIT_OFFSET          12
#define PACKET_TYPE_BIT_MASK            (0xf << PACKET_TYPE_BIT_OFFSET)
#define PACKET_SN_BIT_OFFSET            8
#define PACKET_SN_BIT_MASK              (0xf << PACKET_SN_BIT_OFFSET)
#define PACKET_ENCRYPTION_BIT_OFFSET    7
#define PACKET_ENCRYPTION_BIT_MASK      (0x1 << PACKET_ENCRYPTION_BIT_OFFSET)
#define PACKET_ACK_BIT_OFFSET           6
#define PACKET_ACK_BIT_MASK             (0x1 << PACKET_ACK_BIT_OFFSET)

#define TX_TIMEOUT_DEFAULT              1000
#define ATT_DEFAULT_PAYLOAD_LEN         20        /*! Default maximum payload length for most PDUs */
/*!
 * Macros for converting a little endian byte buffer to integers.
 */
#define BYTES_TO_UINT16(n, p)     {n = ((uint16_t)(p)[0] + ((uint16_t)(p)[1] << 8));}

#define BYTES_TO_UINT32(n, p)     {n = ((uint32_t)(p)[0] + ((uint32_t)(p)[1] << 8) + \
                                        ((uint32_t)(p)[2] << 16) + ((uint32_t)(p)[3] << 24));}
//
// amdtp states
//
typedef enum eAmdtpState
{
    AMDTP_STATE_INIT,
    AMDTP_STATE_TX_IDLE,
    AMDTP_STATE_RX_IDLE,
    AMDTP_STATE_SENDING,
    AMDTP_STATE_GETTING_DATA,
    AMDTP_STATE_WAITING_ACK,
    AMDTP_STATE_MAX
}eAmdtpState_t;


/*! Unknown connection ID or other error */
#define DM_CONN_ID_NONE             0

//
// amdtp packet type
//
typedef enum eAmdtpPktType
{
    AMDTP_PKT_TYPE_UNKNOWN,
    AMDTP_PKT_TYPE_DATA,
    AMDTP_PKT_TYPE_ACK,
    AMDTP_PKT_TYPE_CONTROL,
    AMDTP_PKT_TYPE_MAX
}eAmdtpPktType_t;

typedef enum eAmdtpControl
{
    AMDTP_CONTROL_RESEND_REQ,       // is actually reset command
    AMDTP_CONTROL_MAX
}eAmdtpControl_t;

//
// amdtp status
//
typedef enum eAmdtpStatus
{
    AMDTP_STATUS_SUCCESS,
    AMDTP_STATUS_CRC_ERROR,
    AMDTP_STATUS_INVALID_METADATA_INFO,
    AMDTP_STATUS_INVALID_PKT_LENGTH,
    AMDTP_STATUS_INSUFFICIENT_BUFFER,
    AMDTP_STATUS_UNKNOWN_ERROR,
    AMDTP_STATUS_BUSY,
    AMDTP_STATUS_TX_NOT_READY,              // no connection or tx busy
    AMDTP_STATUS_RESEND_REPLY,
    AMDTP_STATUS_RECEIVE_CONTINUE,
    AMDTP_STATUS_RECEIVE_DONE,
    AMDTP_STATUS_MAX
}eAmdtpStatus_t;

//
// packet prefix structure
//
typedef struct
{
    uint8_t     pktType : 4;
    uint8_t     pktSn   : 4;
    uint8_t     encrypted : 1;
    uint32_t    ackEnabled : 1;
    uint32_t    reserved : 6;               // Reserved for future usage
}
amdtpPktHeader_t;

//
// packet
//
typedef struct
{
    uint16_t            offset;
    uint16_t            len;               // 2 bytes len + 2 bytes header + data plus 4 bytes checksum
    amdtpPktHeader_t    header;
    uint8_t             *data;
}
amdtpPacket_t;

/*! Application data reception callback */
typedef void (*amdtpRecvCback_t)(uint8_t *buf, uint16_t len);

/*! Application data transmission result callback */
typedef void (*amdtpTransCback_t)(eAmdtpStatus_t status);

typedef void (*amdtp_reply_func_t)(eAmdtpStatus_t status, uint8_t *data, uint16_t len);

typedef void (*amdtp_packet_handler_func_t)(eAmdtpPktType_t type, uint16_t len, uint8_t *buf);

typedef eAmdtpStatus_t (*amdtp_ack_sender_func_t)(eAmdtpPktType_t type, gboolean encrypted, gboolean enableACK, uint8_t *buf, uint16_t len);

typedef void (*amdtp_data_sender_func_t)(uint8_t *buf, uint16_t len);

// already defined in #include "../gattrib.h"
typedef void (*GAttribResultFunc) (guint8 status, const guint8 *pdu, guint16 len, gpointer user_data);

typedef struct
{
    eAmdtpState_t   txState;
    eAmdtpState_t   rxState;
    eAmdtpState_t   AckState;
    gboolean        txReady;               // TRUE if ready to send notifications
    amdtpPacket_t   rxPkt;
    amdtpPacket_t   txPkt;
    amdtpPacket_t   ackPkt;
    uint8_t         LastRxPktType;         // hold last received packet type for easier parsing
    uint8_t         TxPktType;             // current transmit package type
    uint8_t         txPktSn;               // data packet serial number for Tx
    eAmdtpPktType_t lastRxPktSn;           // last received data packet serial number
    uint16_t        attMtuSize;            // max MTU size to break long message
    uint16_t        TX_handle;
    uint16_t        RX_handle;
    uint16_t        ACK_handle;
    uint16_t        ACK_CCChandle;
    uint16_t        TX_CCChandle;
    time_t          AckTime;

}
amdtpCb_t;


//*****************************************************************************
//
// function definitions
//
//*****************************************************************************

void
AmdtpBuildPkt(amdtpCb_t *amdtpCb, eAmdtpPktType_t type, gboolean encrypted, gboolean enableACK, uint8_t *buf, uint16_t len);

gboolean
amdtc_init(amdtpCb_t *amdtpCb);

eAmdtpStatus_t
AmdtpReceivePkt(amdtpCb_t *amdtpCb, amdtpPacket_t *pkt, uint16_t len, uint8_t *pValue);

void
AmdtpSendReply(amdtpCb_t *amdtpCb, eAmdtpStatus_t status, uint8_t *data, uint16_t len, gboolean enableACK);

static eAmdtpStatus_t
amdtpcSendAck(amdtpCb_t *amdtpCb, eAmdtpPktType_t type, gboolean encrypted, gboolean enableACK, uint8_t *buf, uint16_t len);

void
AmdtpSendControl(amdtpCb_t *amdtpCb, eAmdtpControl_t control, uint8_t *data, uint16_t len);

void
AmdtpSendPacketHandler(amdtpCb_t *amdtpCb, GAttribResultFunc callback);

void
AmdtpPacketHandler(amdtpCb_t *amdtpCb);

void
resetPkt(amdtpPacket_t *pkt);

#ifdef __cplusplus
}
#endif

#endif // AMDTP_COMMON_H

// ****************************************************************************
//
//  amdtp_main.c
//! @file
//!
//! @brief Ambiq Micro's demonstration of AMDTP service.
//!
//! @{
//
// ****************************************************************************

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

#include <string.h>
#include "wsf_types.h"
#include "bstream.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "hci_api.h"
#include "dm_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_api.h"
#include "app_db.h"
#include "app_ui.h"
#include "app_hw.h"
#include "svc_ch.h"
#include "svc_core.h"
#include "svc_dis.h"
#include "amdtp_api.h"
#include "amdtps_api.h"
#include "svc_amdtp.h"


#include "am_util.h"
#include <unistd.h>   // sleep
#include "debug.h"

// does the driver support security
#ifdef ATT_UUID_DATABASE_HASH 
#define SECURITYENABLED
#endif

#if (defined BLE_Debug) || (defined BLE_SHOW_DATA)
extern void debug_print(const char* f, const char* F, uint16_t L);
extern void debug_float (float f);
#endif
extern void debug_printf(char* fmt, ...);

// call for user level handling of data
extern void UserRequestRec(uint8_t * buf, uint16_t len);

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! WSF message event starting value */
#define AMDTP_MSG_START               0xA0

/*! WSF message event enumeration */
enum
{
  AMDTP_TIMER_IND = AMDTP_MSG_START,  /*! AMDTP tx timeout timer expired */
#ifdef MEASURE_THROUGHPUT   // see ble_debug.h
  AMDTP_MEAS_TP_TIMER_IND,
#endif
};

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Application message type */
typedef union
{
  wsfMsgHdr_t     hdr;
  dmEvt_t         dm;
  attsCccEvt_t    ccc;
  attEvt_t        att;
} amdtpMsg_t;

static void amdtpClose(amdtpMsg_t *pMsg);   // nothing happens here
/**************************************************************************************************
  Configurable Parameters
**************************************************************************************************/

/*! configurable parameters for advertising */
static const appAdvCfg_t amdtpAdvCfg =
{
  {60000,     0,     0},                  /*! Advertising durations in ms */
  {  800,   800,     0}                   /*! Advertising intervals in 0.625 ms units */
};

/*! configurable parameters for slave */
static const appSlaveCfg_t amdtpSlaveCfg =
{
  AMDTP_CONN_MAX,                           /*! Maximum connections */
};

/*! configurable parameters for security */
static const appSecCfg_t amdtpSecCfg =
{
  DM_AUTH_BOND_FLAG | DM_AUTH_SC_FLAG,    /*! Authentication and bonding flags */
  0,                                      /*! Initiator key distribution flags */
  DM_KEY_DIST_LTK,                        /*! Responder key distribution flags */
  FALSE,                                  /*! TRUE if Out-of-band pairing data is present */
  FALSE                                   /*! TRUE to initiate security upon connection */
};

/*! configurable parameters for AMDTP connection parameter update */
static const appUpdateCfg_t amdtpUpdateCfg =
{
  3000,                                   /*! Connection idle period in ms before attempting
                                              connection parameter update; set to zero to disable */
 /* W/A: Apollo2-Blue has issues with interval 7.5ms */
#ifdef AM_PART_APOLLO3
  6,                                      /*! 7.5ms */
  15,                                     /*! 18.75ms */
#else
   8,
  18,
#endif
  0,                                      /*! Connection latency */
  600,                                    /*! Supervision timeout in 10ms units */
  5                                       /*! Number of update attempts before giving up */
};

/*! SMP security parameter configuration */
static const smpCfg_t amdtpSmpCfg =
{
  3000,                                   /*! 'Repeated attempts' timeout in msec */
  SMP_IO_NO_IN_NO_OUT,                    /*! I/O Capability */
  7,                                      /*! Minimum encryption key length */
  16,                                     /*! Maximum encryption key length */
  3,                                      /*! Attempts to trigger 'repeated attempts' timeout */
  0,                                      /*! Device authentication requirements */
};

/*! AMDTPS configuration */
static const AmdtpsCfg_t amdtpAmdtpsCfg =
{
    0
};
/**************************************************************************************************
  Advertising Data
**************************************************************************************************/

/*! advertising data, discoverable mode */
static const uint8_t amdtpAdvDataDisc[] =
{
  /*! flags */
  2,                                      /*! length */
  DM_ADV_TYPE_FLAGS,                      /*! AD type */
  DM_FLAG_LE_GENERAL_DISC |               /*! flags */
  DM_FLAG_LE_BREDR_NOT_SUP,

  /*! tx power */
  2,                                      /*! length */
  DM_ADV_TYPE_TX_POWER,                   /*! AD type */
  0,                                      /*! tx power */

  /*! service UUID list */
  3,                                      /*! length */
  DM_ADV_TYPE_16_UUID,                    /*! AD type */
  UINT16_TO_BYTES(ATT_UUID_DEVICE_INFO_SERVICE),

  17,                                      /*! length */
  DM_ADV_TYPE_128_UUID,                    /*! AD type */
  ATT_UUID_AMDTP_SERVICE
};

/*! advertising data, discoverable mode */
#define MAX_ADV_DATA_LEN 31
uint8_t amdtpScanDataDisc[MAX_ADV_DATA_LEN] = {0};

void set_adv_name( const char* str ){
  uint8_t indi = 0;
  bool done = false;
  do{
    if( *(str+indi) == 0 ){
      done = true;
    }else{
      amdtpScanDataDisc[2+indi] = *(str+indi); // copies characters from str into the data
      indi++;
      if(indi >= (MAX_ADV_DATA_LEN-2)){
        done = true;
      }
    }
  } while(!done);
  amdtpScanDataDisc[0] = 30;                     // sets the 'length' field (note: if the sum of 'length' fields does not match the size of the adv data array then there can be problems!)
  amdtpScanDataDisc[1] = DM_ADV_TYPE_LOCAL_NAME; // sets the 'type' field

  //debug_printf("adv data: %s\n", str);
}

/**************************************************************************************************
  Client Characteristic Configuration Descriptors
**************************************************************************************************/

/*! enumeration of client characteristic configuration descriptors */
enum
{
  AMDTP_GATT_SC_CCC_IDX,                  /*! GATT service, service changed characteristic */
  AMDTP_AMDTPS_TX_CCC_IDX,                /*! AMDTP service, tx characteristic */
  AMDTP_AMDTPS_RX_ACK_CCC_IDX,            /*! AMDTP service, rx ack characteristic */
  AMDTP_NUM_CCC_IDX
};

/*! client characteristic configuration descriptors settings, indexed by above enumeration */
static const attsCccSet_t amdtpCccSet[AMDTP_NUM_CCC_IDX] =
{
  /* cccd handle                value range               security level */
  {GATT_SC_CH_CCC_HDL,          ATT_CLIENT_CFG_INDICATE,  DM_SEC_LEVEL_NONE},   /* AMDTP_GATT_SC_CCC_IDX */
  {AMDTPS_TX_CH_CCC_HDL,        ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},   /* AMDTP_AMDTPS_TX_CCC_IDX */
  {AMDTPS_ACK_CH_CCC_HDL,       ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE}    /* AMDTP_AMDTPS_RX_ACK_CCC_IDX */
};
/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t amdtpHandlerId;

#ifdef MEASURE_THROUGHPUT
wsfTimer_t measTpTimer;
int gTotalDataBytesRecev = 0;
#endif

/*************************************************************************************************/
/*!
 *  \fn     amdtpDmCback
 *
 *  \brief  Application DM callback.
 *
 *  \param  pDmEvt  DM callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpDmCback(dmEvt_t *pDmEvt)
{
  dmEvt_t *pMsg;
  uint16_t len;

  #ifdef BLE_Debug
  //  debug_print(__func__, __FILE__, __LINE__);
  #endif

  len = DmSizeOfEvt(pDmEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pDmEvt, len);
    WsfMsgSend(amdtpHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpAttCback
 *
 *  \brief  Application ATT callback. Sending back to client
 *
 *  \param  pEvt    ATT callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpAttCback(attEvt_t *pEvt)
{
  #ifdef BLE_Debug
    debug_print(__func__, __FILE__, __LINE__);
  #endif

  attEvt_t *pMsg;

  // allocate memory for event structure and (optional) data
  if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attEvt_t));       // cp event
    pMsg->pValue = (uint8_t *) (pMsg + 1);      // set pValue pointer after pMsg
    memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
    WsfMsgSend(amdtpHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpCccCback
 *
 *  \brief  Application ATTS client characteristic configuration callback.
 *
 *  \param  pDmEvt  DM callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpCccCback(attsCccEvt_t *pEvt)
{
  #ifdef BLE_Debug
    debug_print(__func__, __FILE__, __LINE__);
  #endif

  attsCccEvt_t  *pMsg;
  appDbHdl_t    dbHdl;

  /* if CCC not set from initialization and there's a device record */
  if ((pEvt->handle != ATT_HANDLE_NONE) &&
      ((dbHdl = AppDbGetHdl((dmConnId_t) pEvt->hdr.param)) != APP_DB_HDL_NONE))
  {
    /* store value in device database */
    AppDbSetCccTblValue(dbHdl, pEvt->idx, pEvt->value);
  }

  if ((pMsg = WsfMsgAlloc(sizeof(attsCccEvt_t))) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attsCccEvt_t));
    WsfMsgSend(amdtpHandlerId, pMsg);        // this will always call AmdtpHandler
  }
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpProcCccState
 *
 *  \brief  Process CCC state change.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
bool bPairingCompleted = false;
static void amdtpProcCccState(amdtpMsg_t *pMsg)
{
  #ifdef BLE_Debug
    debug_print(__func__, __FILE__, __LINE__);
    debug_printf("======>ccc state ind value:%d handle:%x idx:%d\n", pMsg->ccc.value, pMsg->ccc.handle, pMsg->ccc.idx);
  #endif

  /* AMDTPS TX CCC */
  if (pMsg->ccc.idx == AMDTP_AMDTPS_TX_CCC_IDX)
  {
    if (pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY)
    {
      // notify enabled
      amdtps_start((dmConnId_t)pMsg->ccc.hdr.param, AMDTP_TIMER_IND, AMDTP_AMDTPS_TX_CCC_IDX);
      // AppSlaveSecurityeq(1);
      // if(bPairingCompleted == true)
    }
    else
    {
      // notify disabled
      amdtpClose(pMsg); // nothing happens here
      amdtps_stop((dmConnId_t)pMsg->ccc.hdr.param);
    }
    return;
  }
}

#ifdef MEASURE_THROUGHPUT
static bool measTpStarted = false;

static void showThroughput(void)
{
  #ifdef BLE_Debug
    debug_print(__func__, __FILE__, __LINE__);
  #endif

#if BLE_SHOW_DATA
  debug_printf("throughput : %d Bytes/s\n", gTotalDataBytesRecev);
#endif

  // only restart timer if throughput
  if (gTotalDataBytesRecev != 0)   WsfTimerStartSec(&measTpTimer, 1);
  else measTpStarted = false;

  gTotalDataBytesRecev = 0;
}
#endif //MEASURE_THROUGHPUT

/*************************************************************************************************/
/*!
 *  \fn     amdtpClose
 *
 *  \brief  Perform UI actions on connection close.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpClose(amdtpMsg_t *pMsg)
{
  #ifdef BLE_Debug
  //  debug_print(__func__, __FILE__, __LINE__);
  #endif
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpSetup
 *
 *  \brief  Set up advertising and other procedures that need to be performed after
 *          device reset.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpSetup(amdtpMsg_t *pMsg)
{
  #ifdef BLE_Debug
    debug_print(__func__, __FILE__, __LINE__);
  #endif

  /* set advertising and scan response data for discoverable mode */
  AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(amdtpAdvDataDisc), (uint8_t *) amdtpAdvDataDisc);
  AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(amdtpScanDataDisc), (uint8_t *) amdtpScanDataDisc);

  /* set advertising and scan response data for connectable mode */
  AppAdvSetData(APP_ADV_DATA_CONNECTABLE, sizeof(amdtpAdvDataDisc), (uint8_t *) amdtpAdvDataDisc);
  AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, sizeof(amdtpScanDataDisc), (uint8_t *) amdtpScanDataDisc);

  /* start advertising; automatically set connectable/discoverable mode and bondable mode */
  AppAdvStart(APP_MODE_AUTO_INIT);
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpProcMsg
 *
 *  \brief  Process messages from the event handler.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpProcMsg(amdtpMsg_t *pMsg)
{
  uint8_t D = 0;

  #ifdef BLE_Debug
    debug_printf("%s ",__func__);
    D = 1;
  #endif

  switch(pMsg->hdr.event)
  {
    case AMDTP_TIMER_IND:           /*! AMDTP tx timeout timer expired */
      if (D) debug_printf("AMDTP_TIMER_IND\n");
      amdtps_proc_msg(&pMsg->hdr);
      break;

#ifdef MEASURE_THROUGHPUT
    case AMDTP_MEAS_TP_TIMER_IND:
      if (D) debug_printf("AMDTP_MEAS_TP_TIMER_IND\n");
      showThroughput();
      break;
#endif

    case ATTS_HANDLE_VALUE_CNF:     /*!< \brief Handle value confirmation */
      if (D) debug_printf("ATTS_HANDLE_VALUE_CNF\n");
      amdtps_proc_msg(&pMsg->hdr);
      break;

    case ATTS_CCC_STATE_IND:       /*!< \brief Client chracteristic configuration state change */
      if (D) debug_printf("ATTS_CCC_STATE_IND\n");
      amdtpProcCccState(pMsg);
      break;

    case ATT_MTU_UPDATE_IND:        /*!< \brief Negotiated MTU value */
      if (D) debug_printf("ATT_MTU_UPDATE_IND Negotiated MTU %d \n", ((attEvt_t *)pMsg)->mtu );
      break;

    case DM_CONN_DATA_LEN_CHANGE_IND: /*!< \brief Data length changed */
      if (D) debug_printf("DM_CONN_DATA_LEN_CHANGE_IND\n");
      break;

    case DM_RESET_CMPL_IND:         /*! Reset complete */
      if (D) debug_printf("DM_RESET_CMPL_IND \n");
#ifdef SECURITYENABLED
      AttsCalculateDbHash();
      if (D) debug_printf("hashing started\n");
#endif
      DmSecGenerateEccKeyReq();
      amdtpSetup(pMsg);
      break;

    case DM_ADV_START_IND:          /*! Advertising started */
      if (D) debug_printf("DM_ADV_START_IND\n");
      break;

    case DM_ADV_STOP_IND:           /*! Advertising stopped */
      if (D) debug_printf("DM_ADV_STOP_IND\n");
      break;

    case DM_CONN_OPEN_IND:          /*! Connection opened */
      if (D) debug_printf("DM_CONN_OPEN_IND\n");
      amdtps_proc_msg(&pMsg->hdr);

      // PDU length, TX interval (0x148 ~ 0x848)
      //  \param  connId      Connection identifier. 1
      //  \param  txOctets    Maximum number of payload octets for a Data PDU. 251 (= bytes)
      //  \param  txTime      Maximum number of microseconds for a Data PDU. 0X848
      DmConnSetDataLen(1, 251, 0x848);
      break;

    case DM_CONN_CLOSE_IND:             /*! Connection closed */
      if (D) debug_printf("DM_CONN_CLOSE_IND\n");
      amdtpClose(pMsg); // nothing happens here
      amdtps_proc_msg(&pMsg->hdr);
      break;

    case DM_CONN_UPDATE_IND:               /*! Connection update complete */
      if (D) debug_printf("DM_CONN_UPDATE_IND\n");
      amdtps_proc_msg(&pMsg->hdr);
      break;

    case DM_SEC_PAIR_CMPL_IND:              /*! Pairing completed successfully */
      DmSecGenerateEccKeyReq();
      if (D) debug_printf("DM_SEC_PAIR_CMPL_IND\n");
      bPairingCompleted = true;
      break;

    case DM_SEC_PAIR_FAIL_IND:              /*! Pairing failed or other security failure */
      DmSecGenerateEccKeyReq();
      if (D) debug_printf("DM_SEC_PAIR_FAIL_IND\n");
      bPairingCompleted = false;
      break;

    case DM_SEC_ENCRYPT_IND:                /*! Connection encrypted */
      if (D) debug_printf("DM_SEC_ENCRYPT_IND\n");
      break;

    case DM_SEC_ENCRYPT_FAIL_IND:           /*! Encryption failed */
      if (D) debug_printf("DM_SEC_ENCRYPT_FAIL_IND\n");
      break;

    case DM_SEC_AUTH_REQ_IND:               /*! PIN or OOB data requested for pairing */
      if (D) debug_printf("DM_SEC_AUTH_REQ_IND\n");
      AppHandlePasskey(&pMsg->dm.authReq);
      break;

    case DM_SEC_ECC_KEY_IND:                /*!< \brief Result of ECC Key Generation */
      if (D) debug_printf("DM_SEC_ECC_KEY_IND\n");
      DmSecSetEccKey(&pMsg->dm.eccMsg.data.key);
      amdtpSetup(pMsg);                     // Set up advertising and other procedures after reset
      break;

    case DM_SEC_COMPARE_IND:                /*!< \brief Result of Just Works/Numeric Comparison Compare Value Calculation */
      if (D) debug_printf("DM_SEC_COMPARE_IND\n");
      AppHandleNumericComparison(&pMsg->dm.cnfInd);
      break;

    case DM_ERROR_IND:                      /*! general error, added for debug only as nothing is happening */
      if (D) debug_printf("DM_ERROR_IND\n");
      break;

    case DM_VENDOR_SPEC_CMD_CMPL_IND:       /*! Vendor specific command complete event */
      {
        if (D) debug_printf("DM_VENDOR_SPEC_CMD_CMPL_IND\n");
        #if defined(AM_PART_APOLLO) || defined(AM_PART_APOLLO2)

          uint8_t *param_ptr = &pMsg->dm.vendorSpecCmdCmpl.param[0];

          switch (pMsg->dm.vendorSpecCmdCmpl.opcode)
          {
            case 0xFC20: //read at address
            {
              uint32_t read_value;

              BSTREAM_TO_UINT32(read_value, param_ptr);

              //debug_printf("VSC 0x%0x complete status %x param %x",
                pMsg->dm.vendorSpecCmdCmpl.opcode,
                pMsg->hdr.status,
                read_value);
            }

            break;
            default:
                //debug_printf("VSC 0x%0x complete status %x",
                    pMsg->dm.vendorSpecCmdCmpl.opcode,
                    pMsg->hdr.status);
            break;
          }

        #endif
      }
      break;

    default:
      if (D) debug_printf("DEFAULT event: 0x%x, dec %d\n", pMsg->hdr.event, pMsg->hdr.event);
      break;
  }
}


// called when complete data has been received and retrieved
void amdtpDtpRecvCback(uint8_t * buf, uint16_t len)
{
#ifdef BLE_Debug
   // debug_print(__func__, __FILE__, __LINE__);
#endif

#ifdef BLE_SHOW_DATA
    debug_printf("-----------AMDTP Received data--------------\n");
    debug_printf("len = %d\n", len);
    for (uint16_t i = 0; i < len; i++) debug_printf("buf [%d] : 0x%02X ", i, buf[i]);
    debug_printf("\n");
#endif

    // call user interface program
    UserRequestRec(buf, len);

#ifdef MEASURE_THROUGHPUT
    gTotalDataBytesRecev += len;
    if (!measTpStarted)
    {
        measTpStarted = true;
        WsfTimerStartSec(&measTpTimer, 1);
    }
#endif
}


// call back when data has been transferred
void amdtpDtpTransCback(eAmdtpStatus_t status)
{
    #ifdef BLE_Debug
     debug_print(__func__, __FILE__, __LINE__);
    #endif

    if (status == AMDTP_STATUS_SUCCESS )
    {
        // ..
    }
}

/*************************************************************************************************/
/*!
 *  \fn     AmdtpHandlerInit
 *
 *  \brief  Application handler init function called during system initialization.
 *
 *  \param  handlerID  WSF handler ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AmdtpHandlerInit(wsfHandlerId_t handlerId)
{
  #ifdef BLE_Debug
    debug_print(__func__, __FILE__, __LINE__);
  #endif
  /* store handler ID */
  amdtpHandlerId = handlerId;

  /* Set configuration pointers */
  pAppAdvCfg = (appAdvCfg_t *) &amdtpAdvCfg;
  pAppSlaveCfg = (appSlaveCfg_t *) &amdtpSlaveCfg;
  pAppSecCfg = (appSecCfg_t *) &amdtpSecCfg;
  pAppUpdateCfg = (appUpdateCfg_t *) &amdtpUpdateCfg;

  /* Initialize application framework */
  AppSlaveInit();

  /* Set stack configuration pointers */
  pSmpCfg = (smpCfg_t *) &amdtpSmpCfg;

  /* initialize amdtp service server */
  amdtps_init(handlerId, (AmdtpsCfg_t *) &amdtpAmdtpsCfg, amdtpDtpRecvCback, amdtpDtpTransCback);

#ifdef MEASURE_THROUGHPUT
  measTpTimer.handlerId = handlerId;
  measTpTimer.msg.event = AMDTP_MEAS_TP_TIMER_IND;
#endif
}

/*************************************************************************************************/
/*!
 *  \fn     AmdtpHandler
 *
 *  \brief  WSF event handler for application.
 *
 *  \param  event   WSF event mask.
 *  \param  pMsg    WSF message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AmdtpHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  #ifdef BLE_Debug
    //debug_print(__func__, __FILE__, __LINE__);
  #endif

  if (pMsg != NULL)
  {

    if (pMsg->event >= DM_CBACK_START && pMsg->event <= DM_CBACK_END)
    {
      /* process advertising and connection-related messages */
      AppSlaveProcDmMsg((dmEvt_t *) pMsg);

      /* process security-related messages */
      AppSlaveSecProcDmMsg((dmEvt_t *) pMsg);
    }

    /* perform profile and user interface-related operations */
    amdtpProcMsg((amdtpMsg_t *) pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \fn     AmdtpStart
 *
 *  \brief  Start the application.
 *
 *  \return None.
 * called frim the .ino to setup
 */
/*************************************************************************************************/
void AmdtpStart(void)
{
  #ifdef BLE_Debug
    debug_print(__func__, __FILE__, __LINE__);
  #endif

  /* Register for stack callbacks */
  DmRegister(amdtpDmCback);
  DmConnRegister(DM_CLIENT_ID_APP, amdtpDmCback);   // Identifier for the application
  AttRegister(amdtpAttCback);
  AttConnRegister(AppServerConnCback);
  AttsCccRegister(AMDTP_NUM_CCC_IDX, (attsCccSet_t *) amdtpCccSet, amdtpCccCback);

  /* Initialize attribute server database */
  SvcCoreAddGroup();
  SvcDisAddGroup();                                 // device information.. could be left out
  SvcAmdtpsCbackRegister(NULL, amdtps_write_cback);
  SvcAmdtpsAddGroup();

  /* Reset the device */
  DmDevReset();
}

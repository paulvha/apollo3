// ****************************************************************************
//
//  amdtp_main.c
//! @file
//!
//! @brief Ambiq Micro's demonstration of AMDTP client.
//!
//! @{
//
// ****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2020, Ambiq Micro
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
// This is part of revision 2.4.2 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#include <string.h>
#include "wsf_types.h"
#include "bstream.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "hci_api.h"
#include "dm_api.h"
#include "gap_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_cfg.h"
#include "app_api.h"
#include "app_db.h"
#include "app_ui.h"
#include "svc_core.h"
#include "svc_ch.h"
#include "gatt_api.h"
#include "amdtp_api.h"
#include "amdtpc_api.h"
#include "calc128.h"
#include "ble_menu.h"
#include "gatt_api.h"

//extra includes
#include "svc_amdtp.h"
#include <stdbool.h>
#include "amdtp_svrcmds.h"

/*************************************************************************************************
Macros
**************************************************************************************************/

// enable debug for this file
#ifdef AM_DEBUG_PRINTF
#define AMDTP_MAIN_DEBUG
#endif

// does the driver support security ?
#ifdef ATT_UUID_DATABASE_HASH
#define SECURITYENABLED
#endif

/*! WSF message event starting value */
#define AMDTP_MSG_START       0xA0

/*! WSF message event enumeration */
enum
{
  AMDTP_TIMER_IND = AMDTP_MSG_START,    /*! AMDTP tx timeout timer expired */
  AMDTP_MEAS_TP_TIMER_IND,             // Throughput timer
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

// enable throughput measurement
bool MEASURE_THROUGHPUT = false;

/*! application control block */
struct
{
  uint16_t          hdlList[APP_DB_HDL_LIST_LEN];   /*! Cached handle list */
  wsfHandlerId_t    handlerId;                      /*! WSF hander ID */
  bool_t            scanning;                       /*! TRUE if scanning */
  bool_t            autoConnect;                    /*! TRUE if auto-connecting */
  uint8_t           discState;                      /*! Service discovery state */
  uint8_t           hdlListLen;                     /*! Cached handle list length */
} amdtpcCb;

/*! connection control block */
typedef struct
{
  appDbHdl_t          dbHdl;                        /*! Device database record handle type */
  uint8_t             addrType;                     /*! Type of address of device to connect to */
  bdAddr_t            addr;                         /*! Address of device to connect to */
  bool_t              doConnect;                    /*! TRUE to issue connect on scan complete */
} amdtpcConnInfo_t;

amdtpcConnInfo_t amdtpcConnInfo;

// store names scan discovered devices
#define DEVICENAMELEN 30
#define MAXDEVICEINFO 5
struct
{
  bdAddr_t            addr;                         /*! Address of device to connect to */
  char                dname[DEVICENAMELEN];
} scaninfo[MAXDEVICEINFO];

// keep track of entries
int infocnt = 0;

/**************************************************************************************************
  Configurable Parameters
**************************************************************************************************/

/*! configurable parameters for master */
static const appMasterCfg_t amdtpcMasterCfg =
{
  96,                                      /*! The scan interval, in 0.625 ms units */
  48,                                      /*! The scan window, in 0.625 ms units  */
  4000,                                    /*! The scan duration in ms */
  DM_DISC_MODE_NONE,                       /*! The GAP discovery mode */
  DM_SCAN_TYPE_ACTIVE                      /*! The scan type (active or passive) */
};

/*! configurable parameters for security */
static const appSecCfg_t amdtpcSecCfg =
{
  DM_AUTH_BOND_FLAG | DM_AUTH_SC_FLAG,    /*! Authentication and bonding flags */
  DM_KEY_DIST_IRK,                        /*! Initiator key distribution flags */
  DM_KEY_DIST_LTK | DM_KEY_DIST_IRK,      /*! Responder key distribution flags */
  FALSE,                                  /*! TRUE if Out-of-band pairing data is present */
  FALSE                                   /*! TRUE to initiate security upon connection */
};

/*! SMP security parameter configuration */
static const smpCfg_t amdtpcSmpCfg =
{
  3000,                                   /*! 'Repeated attempts' timeout in msec */
  SMP_IO_NO_IN_NO_OUT,                    /*! I/O Capability */
  7,                                      /*! Minimum encryption key length */
  16,                                     /*! Maximum encryption key length */
  3,                                      /*! Attempts to trigger 'repeated attempts' timeout */
  0,                                      /*! Device authentication requirements */
};

/*! Connection parameters */
static const hciConnSpec_t amdtpcConnCfg =
{
  40,                                     /*! Minimum connection interval in 1.25ms units */
  40,                                     /*! Maximum connection interval in 1.25ms units */
  0,                                      /*! Connection latency */
  600,                                    /*! Supervision timeout in 10ms units */
  0,                                      /*! Unused */
  0                                       /*! Unused */
};

/*! Configurable parameters for service and characteristic discovery */
static const appDiscCfg_t amdtpcDiscCfg =
{
  FALSE                                   /*! TRUE to wait for a secure connection before initiating discovery */
};

static const appCfg_t amdtpcAppCfg =
{
  TRUE,                                   /*! TRUE to abort service discovery if service not found */
  TRUE                                    /*! TRUE to disconnect if ATT transaction times out */
};

/*! local IRK */
static uint8_t localIrk[] =
{
  0xA6, 0xD9, 0xFF, 0x70, 0xD6, 0x1E, 0xF0, 0xA4, 0x46, 0x5F, 0x8D, 0x68, 0x19, 0xF3, 0xB4, 0x96
};

/**************************************************************************************************
  ATT Client Discovery Data
**************************************************************************************************/

/*! Discovery states:  enumeration of services to be discovered */
enum
{
  AMDTPC_DISC_GATT_SVC,      /*! GATT service */
  AMDTPC_DISC_GAP_SVC,       /*! GAP service */
  AMDTPC_DISC_AMDTP_SVC,     /*! AMDTP service */
  AMDTPC_DISC_SVC_MAX        /*! Discovery complete */
};

/*! the Client handle list, amdtpcCb.hdlList[], is set as follows:
 *
 *  ------------------------------- <- AMDTPC_DISC_GATT_START
 *  | GATT svc changed handle     |
 *  -------------------------------
 *  | GATT svc changed ccc handle |
 *  ------------------------------- <- AMDTPC_DISC_GAP_START
 *  | GAP central addr res handle |
 *  -------------------------------
 *  | GAP RPA Only handle         |
 *  ------------------------------- <- AMDTPC_DISC_AMDTP_START
 *  | WP handles                  |
 *  | ...                         |
 *  -------------------------------
 */

/*! Start of each service's handles in the the handle list */
#define AMDTPC_DISC_GATT_START     0
#define AMDTPC_DISC_GAP_START      (AMDTPC_DISC_GATT_START + GATT_HDL_LIST_LEN)
#define AMDTPC_DISC_AMDTP_START    (AMDTPC_DISC_GAP_START + GAP_HDL_LIST_LEN)
#define AMDTPC_DISC_HDL_LIST_LEN   (AMDTPC_DISC_AMDTP_START + AMDTP_HDL_LIST_LEN)

/*! Pointers into handle list for each service's handles */
static uint16_t *pAmdtpcGattHdlList = &amdtpcCb.hdlList[AMDTPC_DISC_GATT_START];
static uint16_t *pAmdtpcGapHdlList = &amdtpcCb.hdlList[AMDTPC_DISC_GAP_START];
static uint16_t *pAmdtpcHdlList = &amdtpcCb.hdlList[AMDTPC_DISC_AMDTP_START];


/* LESC OOB configuration */
static dmSecLescOobCfg_t *amdtpcOobCfg;

/**************************************************************************************************
  ATT Client Configuration Data
**************************************************************************************************/

/*
 * Data for configuration after service discovery
 */

/* Default value for CCC indications */
const uint8_t amdtpcCccIndVal[2] = {UINT16_TO_BYTES(ATT_CLIENT_CFG_INDICATE)};

/* Default value for CCC notifications */
const uint8_t amdtpcTxCccNtfVal[2] = {UINT16_TO_BYTES(ATT_CLIENT_CFG_NOTIFY)};

/* Default value for CCC notifications */
const uint8_t amdtpcAckCccNtfVal[2] = {UINT16_TO_BYTES(ATT_CLIENT_CFG_NOTIFY)};

/* List of characteristics to configure after service discovery */
static const attcDiscCfg_t amdtpcDiscCfgList[] =
{
  /* Write:  GATT service changed ccc descriptor */
  {amdtpcCccIndVal, sizeof(amdtpcCccIndVal), (GATT_SC_CCC_HDL_IDX + AMDTPC_DISC_GATT_START)},

  /* Write:  AMDTP service TX ccc descriptor */
  {amdtpcTxCccNtfVal, sizeof(amdtpcTxCccNtfVal), (AMDTP_TX_DATA_CCC_HDL_IDX + AMDTPC_DISC_AMDTP_START)},

  /* Write:  AMDTP service TX ccc descriptor */
  {amdtpcAckCccNtfVal, sizeof(amdtpcAckCccNtfVal), (AMDTP_ACK_CCC_HDL_IDX + AMDTPC_DISC_AMDTP_START)},
};

/* Characteristic configuration list length */
#define AMDTPC_DISC_CFG_LIST_LEN   (sizeof(amdtpcDiscCfgList) / sizeof(attcDiscCfg_t))

/* sanity check:  make sure configuration list length is <= handle list length */
//WSF_ASSERT(AMDTPC_DISC_CFG_LIST_LEN <= AMDTPC_DISC_HDL_LIST_LEN);

// for Throughput mesaurement (can be enable or disabled by client)
wsfTimer_t measTpTimer;
int gTotalDataBytesRecev = 0;
bool measTpStarted = false;

/*************************************************************************************************/
/*!
 *  \fn     amdtpcDmCback
 *
 *  \brief  Application DM callback.
 *
 *  \param  pDmEvt  DM callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpcDmCback(dmEvt_t *pDmEvt)
{
  dmEvt_t   *pMsg;
  uint16_t  len;
  uint16_t  reportLen;

  if (pDmEvt->hdr.event == DM_SEC_ECC_KEY_IND)
  {
#ifdef AMDTP_MAIN_DEBUG
    am_menu_printf("%s  DM_SEC_ECC_KEY_IND\r\n",__func__);
    am_util_debug_printf("%s end\n",__func__);
am_util_debug_printf("%s end1\n",__func__);
am_util_debug_printf("%s end2\n",__func__);
am_util_debug_printf("%s end3\n",__func__);
am_util_debug_printf("%s end4\n",__func__);
#endif
    DmSecSetEccKey(&pDmEvt->eccMsg.data.key);

    if (amdtpcSecCfg.oob)
    {
      uint8_t oobLocalRandom[SMP_RAND_LEN];
      SecRand(oobLocalRandom, SMP_RAND_LEN);
      DmSecCalcOobReq(oobLocalRandom, pDmEvt->eccMsg.data.key.pubKey_x);
    }
  }
  else if (pDmEvt->hdr.event == DM_SEC_CALC_OOB_IND)
  {
#ifdef AMDTP_MAIN_DEBUG
    am_menu_printf("%s  DM_SEC_CALC_OOB_IND\r\n",__func__);
#endif
    if (amdtpcOobCfg == NULL)
    {
      amdtpcOobCfg = WsfBufAlloc(sizeof(dmSecLescOobCfg_t));
    }

    if (amdtpcOobCfg)
    {
      Calc128Cpy(amdtpcOobCfg->localConfirm, pDmEvt->oobCalcInd.confirm);
      Calc128Cpy(amdtpcOobCfg->localRandom, pDmEvt->oobCalcInd.random);
    }
  }
  else
  {
    len = DmSizeOfEvt(pDmEvt);

    if (pDmEvt->hdr.event == DM_SCAN_REPORT_IND)
    {
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("%s DM_SCAN_REPORT_IND\r\n",__func__);
#endif
      reportLen = pDmEvt->scanReport.len;
    }
    else
    {
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("%s OTHER REASON\r\n",__func__);
#endif
      reportLen = 0;
    }

    if ((pMsg = WsfMsgAlloc(len + reportLen)) != NULL)
    {
      memcpy(pMsg, pDmEvt, len);
      if (pDmEvt->hdr.event == DM_SCAN_REPORT_IND)
      {
        pMsg->scanReport.pData = (uint8_t *) ((uint8_t *) pMsg + len);
        memcpy(pMsg->scanReport.pData, pDmEvt->scanReport.pData, reportLen);
      }
      WsfMsgSend(amdtpcCb.handlerId, pMsg);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpcAttCback
 *
 *  \brief  Application  ATT callback.
 *
 *  \param  pEvt    ATT callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpcAttCback(attEvt_t *pEvt)
{
  attEvt_t *pMsg;

  if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attEvt_t));
    pMsg->pValue = (uint8_t *) (pMsg + 1);
    memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
    WsfMsgSend(amdtpcCb.handlerId, pMsg);

#ifdef AMDTP_MAIN_DEBUG
    am_menu_printf("%s call back to AmdtpcHandler\r\n",__func__);
#endif
  }
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpcScanStart
 *
 *  \brief  Perform actions on scan start.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpcScanStart(dmEvt_t *pMsg)
{

#ifdef AMDTP_MAIN_DEBUG
  am_menu_printf("%s: call back started \r\n", __func__);
#endif

  if (pMsg->hdr.status == HCI_SUCCESS)
  {
    amdtpcCb.scanning = TRUE;
#ifdef BLE_MENU
    // let menu structure know
    menuRxDataLen = 0;
    add_menu_input(NOT_SCAN_START);
    add_menu_input('\r');
#endif

  }
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpcScanStop
 *
 *  \brief  Perform actions on scan stop.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpcScanStop(dmEvt_t *pMsg)
{
  if (pMsg->hdr.status == HCI_SUCCESS)
  {
    amdtpcCb.scanning = FALSE;
    amdtpcCb.autoConnect = FALSE;

#ifdef BLE_MENU
    // let menu structure know
    menuRxDataLen = 0;
    add_menu_input(NOT_SCAN_STOP);
    add_menu_input('\r');
#endif

    /* Open connection */
    if (amdtpcConnInfo.doConnect)
    {
      AppConnOpen(amdtpcConnInfo.addrType, amdtpcConnInfo.addr, amdtpcConnInfo.dbHdl);
      amdtpcConnInfo.doConnect = FALSE;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpcScanReport
 *
 *  \brief  Handle a scan report.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpcScanReport(dmEvt_t *pMsg)
{
  uint8_t *pData, i, j;
  appDbHdl_t dbHdl;
  bool_t  connect = FALSE;
  char unk[] = "UNKNOWN";

  // set offset to friendly, name indicator and length name
  char *friendly = 30 + (char *) pMsg;
  uint8_t name_ind = *(29 + (char *) pMsg);
  uint8_t max_length = *(28 + (char *) pMsg);

#ifdef AMDTP_MAIN_DEBUG
  am_menu_printf("%s %d %s\r\n", __func__, infocnt, friendly);
#endif

  /* capture name & peer address. */

  // check whether address was already captured, overwrite in that case
  for (j = 0; j < infocnt; j++) {
    if (BdaCmp(scaninfo[j].addr, pMsg->scanReport.addr)) break;
  }
  // copy bluetooth address
  memcpy(scaninfo[j].addr, pMsg->scanReport.addr, sizeof(bdAddr_t));

  // check for valid name indicator
  if (name_ind != DM_ADV_TYPE_LOCAL_NAME && name_ind != DM_ADV_TYPE_SHORT_NAME)
  {
    friendly = unk;           // set unknown
    max_length = sizeof(unk);
  }

  // copy name
  for (i = 0; i < DEVICENAMELEN && i < max_length ; i++) {
    scaninfo[j].dname[i] = *(friendly + i);
    if (scaninfo[j].dname[i] < 0x20) break;
    //am_menu_printf("%d add %c \r\n", j, scaninfo[j].dname[i]);
  }

  // terminate (in case name was longer than DEVICENAMELEN
  scaninfo[j].dname[i] = 0x0;

  // entry was added to list
  if (j == infocnt) {
    infocnt++
    
    // if more discovered than buffer, overwrite last entry (fix Version 2.0.1)
    if (infocnt == MAXDEVICEINFO ) infocnt--;
  }
  
  /* disregard if not scanning or autoconnecting */
  if (!amdtpcCb.scanning || !amdtpcCb.autoConnect)
  {
    return;
  }

  /* if we already have a bond with this device then connect to it */
  if ((dbHdl = AppDbFindByAddr(pMsg->scanReport.addrType, pMsg->scanReport.addr)) != APP_DB_HDL_NONE)
  {
    /* if this is a directed advertisement where the initiator address is an RPA */
    if (DM_RAND_ADDR_RPA(pMsg->scanReport.directAddr, pMsg->scanReport.directAddrType))
    {
      /* resolve direct address to see if it's addressed to us */
      AppMasterResolveAddr(pMsg, dbHdl, APP_RESOLVE_DIRECT_RPA);
    }
    else
    {
      connect = TRUE;
    }
  }
  /* if the peer device uses an RPA */
  else if (DM_RAND_ADDR_RPA(pMsg->scanReport.addr, pMsg->scanReport.addrType))
  {
    /* reslove advertiser's RPA to see if we already have a bond with this device */
    AppMasterResolveAddr(pMsg, APP_DB_HDL_NONE, APP_RESOLVE_ADV_RPA);
  }
  /* find vendor-specific advertising data */
  else if ((pData = DmFindAdType(DM_ADV_TYPE_MANUFACTURER, pMsg->scanReport.len,
                                 pMsg->scanReport.pData)) != NULL)
  {
    /* check length and vendor ID */
    if (pData[DM_AD_LEN_IDX] >= 3 && BYTES_UINT16_CMP(&pData[DM_AD_DATA_IDX], HCI_ID_ARM))
    {
      connect = TRUE;
    }
  }

  if (connect)
  {
    /* stop scanning and connect */
    amdtpcCb.autoConnect = FALSE;
    AppScanStop();

    /* Store peer information for connect on scan stop */
    amdtpcConnInfo.addrType = pMsg->scanReport.addrType;
    memcpy(amdtpcConnInfo.addr, pMsg->scanReport.addr, sizeof(bdAddr_t));
    amdtpcConnInfo.dbHdl = dbHdl;
    amdtpcConnInfo.doConnect = TRUE;
  }
}
/*************************************************************************************************/
/*!
 *  \fn     get_friendly_name
 *
 *  \brief  check for friendly name
 *
 *  \param  addr     peer device address
 *  \param  friendly buffer to store name
 *  \param  len      length of buffer
 *
 *  \return None.
 */
/*************************************************************************************************/
void get_friendly_name(bdAddr_t addr, char * friendly, uint8_t len)
{
  uint8_t j, cp = 0;
  bool_t  found = FALSE;

  // check for match on address
  for (j = 0; j < infocnt; j++) {

    if (BdaCmp(scaninfo[j].addr, addr)) {
      found = TRUE;
      break;
    }
  }

  // copy name if found
  if (found) {
    for (cp = 0; cp < len -1; cp++) {
      friendly[cp] = scaninfo[j].dname[cp];
      if (friendly[cp] == 0x0) return;
      if (friendly[cp] < 0x20 ) friendly[cp] = '?'; // remove non-printable
    }
  }

  // terminate (in case not found or length exceeded)
  friendly[cp] = 0x0;
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpcOpen
 *
 *  \brief  Perform UI actions on connection open.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpcOpen(dmEvt_t *pMsg)
{
    // action moved amdtpc_start AFTER MTU has been agreed.
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpcSetup
 *
 *  \brief  Set up procedures that need to be performed after device reset.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpcSetup(dmEvt_t *pMsg)
{
  amdtpcCb.scanning = FALSE;
  amdtpcCb.autoConnect = FALSE;
  amdtpcConnInfo.doConnect = FALSE;

  DmConnSetConnSpec((hciConnSpec_t *) &amdtpcConnCfg);
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpcDiscGapCmpl
 *
 *  \brief  GAP service discovery has completed.
 *
 *  \param  connId    Connection identifier.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpcDiscGapCmpl(dmConnId_t connId)
{
  appDbHdl_t dbHdl;

  /* if RPA Only attribute found on peer device */
  if ((pAmdtpcGapHdlList[GAP_RPAO_HDL_IDX] != ATT_HANDLE_NONE) &&
      ((dbHdl = AppDbGetHdl(connId)) != APP_DB_HDL_NONE))
  {
    /* update DB */
    AppDbSetPeerRpao(dbHdl, TRUE);
  }
}

void AmdtpcScanStart(void)
{
  // reset friendly name capture entry pointer
  infocnt = 0;

  AppScanStart(amdtpcMasterCfg.discMode, amdtpcMasterCfg.scanType,
                     amdtpcMasterCfg.scanDuration);
}

void AmdtpcScanStop(void)
{
  AppScanStop();
}

void AmdtpcConnOpen(uint8_t idx)
{
    appDevInfo_t *devInfo;
    devInfo = AppScanGetResult(idx);
    if (devInfo)
    {
        AppConnOpen(devInfo->addrType, devInfo->addr, NULL);
    }
    else
    {
#ifdef AMDTP_MAIN_DEBUG
        am_menu_printf("AmdtpcConnOpen() devInfo = NULL\r\n");
#endif 
    }
}

// close open connection
void AmdtpcConnClose()
{
    dmConnId_t connId;

    if ((connId = AppConnIsOpen()) != DM_CONN_ID_NONE)
    {
#ifdef AMDTP_MAIN_DEBUG
    am_menu_printf("%s calling AppConnClose\r\n",__func__);
#endif
        AppConnClose(connId);
    }
    else
    {
#ifdef AMDTP_MAIN_DEBUG
    am_menu_printf("%s calling amdtpc_conn_close\r\n",__func__);
#endif
        // perform reset anyway
        amdtpc_conn_close(NULL);
    }
}

void amdtpDtpTransCback(eAmdtpStatus_t status)
{
#ifdef AMDTP_MAIN_DEBUG
    am_menu_printf("amdtpDtpTransCback status = %d\r\n", status);
#endif
    if (status == AMDTP_STATUS_SUCCESS)
    {
       //....
    }
}

// call back for received data 
void amdtpDtpRecvCback(uint8_t * buf, uint16_t len)
{
#ifdef AMDTP_MAIN_DEBUG
    am_menu_printf("%s\r\n", __func__);
    am_menu_printf("-----------AMDTP Received data--------------\r\n");
    am_menu_printf("len = %d, buf[0] = %d, buf[1] = %d\r\n", len, buf[0], buf[1]);
    
    int j;
    for( j = 0; j < len ; j ++) {
      if (buf[j] < 0x20) am_menu_printf("%d  0x%X %c\r\n", j, buf[j],buf[j]);
      else  am_menu_printf("%d  0x%X **\r\n", j, buf[j]);
    }
#endif
          
  if (MEASURE_THROUGHPUT) {
          
      gTotalDataBytesRecev += len;
      if (!measTpStarted)
      {
          measTpStarted = true;
          WsfTimerStartSec(&measTpTimer, 1);
      }
  }
    
    // handle response from server
    HandeServerResp(buf, len);
}


/*************************************************************************************************/
/*!
 *  \fn     amdtpcDiscCback
 *
 *  \brief  Discovery callback.
 *
 *  \param  connId    Connection identifier.
 *  \param  status    Service or configuration status.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpcDiscCback(dmConnId_t connId, uint8_t status)
{
  switch(status)
  {
    case APP_DISC_INIT:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("%s APP_DISC_INIT\r\n", __func__);
#endif
      /* set handle list when initialization requested */
      AppDiscSetHdlList(connId, amdtpcCb.hdlListLen, amdtpcCb.hdlList);
      break;

    case APP_DISC_SEC_REQUIRED:
      /* initiate security */
      AppMasterSecurityReq(connId);
      break;
      
#ifdef SECURITYENABLED
    case APP_DISC_READ_DATABASE_HASH:
      /* Read peer's database hash */
      AppDiscReadDatabaseHash(connId);
      break;
#endif

    case APP_DISC_START:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("################### %s APP_DISC_START\r\n", __func__);
#endif
      /* initialize discovery state */
      amdtpcCb.discState = AMDTPC_DISC_GATT_SVC;

      /* discover GATT service */
      GattDiscover(connId, pAmdtpcGattHdlList);
      break;

    case APP_DISC_FAILED:
      if (pAppCfg->abortDisc)
      {
        /* if discovery failed for proprietary data service then disconnect */
        if (amdtpcCb.discState == AMDTPC_DISC_AMDTP_SVC)
        {
#ifdef AMDTP_MAIN_DEBUG
          am_menu_printf("%s APP_DISC_FAILED\r\n", __func__);
#endif
          AppConnClose(connId);
          break;
        }
      }
      /* else fall through and continue discovery */

    case APP_DISC_CMPL:

      /* next discovery state */
      amdtpcCb.discState++;
      
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("%s discovery complete for phase %d\r\n", __func__, amdtpcCb.discState);
#endif
      if (amdtpcCb.discState == AMDTPC_DISC_GAP_SVC)
      {
#ifdef AMDTP_MAIN_DEBUG
        am_menu_printf("################### %s discovery AMDTPC_DISC_GAP_SVC\r\n", __func__);
#endif
        /* discover GAP service */
        GapDiscover(connId, pAmdtpcGapHdlList);
      }
      else if (amdtpcCb.discState == AMDTPC_DISC_AMDTP_SVC)
      {
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("####################### %s discovery AMDTPC_DISC_AMDTP_SVC\r\n", __func__);
#endif
        /* discover proprietary data service */
        AmdtpcDiscover(connId, pAmdtpcHdlList);
      }
      else
      {
#ifdef AMDTP_MAIN_DEBUG
        am_menu_printf("##################### %s discovery complete\r\n", __func__);
#endif
        /* discovery complete */
        AppDiscComplete(connId, APP_DISC_CMPL);

        /* GAP service discovery completed */
        amdtpcDiscGapCmpl(connId);

        /* start configuration */
        AppDiscConfigure(connId, APP_DISC_CFG_START, AMDTPC_DISC_CFG_LIST_LEN,
                         (attcDiscCfg_t *) amdtpcDiscCfgList, AMDTPC_DISC_HDL_LIST_LEN, amdtpcCb.hdlList);
      }
      break;

    case APP_DISC_CFG_START:
        /* start configuration */
      AppDiscConfigure(connId, APP_DISC_CFG_START, AMDTPC_DISC_CFG_LIST_LEN,
                         (attcDiscCfg_t *) amdtpcDiscCfgList, AMDTPC_DISC_HDL_LIST_LEN, amdtpcCb.hdlList);
      break;

    case APP_DISC_CFG_CMPL:
      AppDiscComplete(connId, status);
      amdtpc_start(pAmdtpcHdlList[AMDTP_RX_HDL_IDX], pAmdtpcHdlList[AMDTP_TX_DATA_HDL_IDX], pAmdtpcHdlList[AMDTP_ACK_HDL_IDX], AMDTP_TIMER_IND);
      break;

    case APP_DISC_CFG_CONN_START:
      /* no connection setup configuration */
      break;

    default:
      break;
  }
}


static void showThroughput(void)
{
    am_menu_printf("throughput : %d Bytes/s\r\n", gTotalDataBytesRecev);

    // only restart timer if throughput
    if (gTotalDataBytesRecev != 0)   WsfTimerStartSec(&measTpTimer, 1);
    else measTpStarted = false;
    
    gTotalDataBytesRecev = 0;
}


/*************************************************************************************************/
/*!
 *  \fn     amdtpcProcMsg
 *
 *  \brief  Process messages from the event handler.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void amdtpcProcMsg(dmEvt_t *pMsg)
{

#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("%s event %d ",__func__, pMsg->hdr.event);
#endif

  switch(pMsg->hdr.event)
  {
    case AMDTP_TIMER_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("AMDTP_TIMER_IND\r\n");
#endif
      amdtpc_proc_msg(&pMsg->hdr);
      break;

    case AMDTP_MEAS_TP_TIMER_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("AMDTP_MEAS_TP_TIMER_IND\r\n");
#endif
      if (MEASURE_THROUGHPUT) showThroughput();
      break;

    case ATTC_HANDLE_VALUE_NTF:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("ATTC_HANDLE_VALUE_NTF\r\n");
#endif
      amdtpc_proc_msg(&pMsg->hdr);
      break;

    case ATTC_WRITE_CMD_RSP:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("ATTC_WRITE_CMD_RSP\r\n");
#endif
      amdtpc_proc_msg(&pMsg->hdr);
      break;

    case DM_RESET_CMPL_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_RESET_CMPL_IND\r\n");
#endif

#ifdef SECURITYENABLED
      AttsCalculateDbHash();
#endif
      DmSecGenerateEccKeyReq();
      amdtpcSetup(pMsg);
      BleMenuInit();
      break;

    case DM_SCAN_START_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("scanning started\r\n");
#endif
      amdtpcScanStart(pMsg);
      break;

    case DM_SCAN_STOP_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("scanning stop\r\n");
#endif
      amdtpcScanStop(pMsg);
      break;

    case DM_SCAN_REPORT_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_SCAN_REPORT_IND\r\n");
#endif
      amdtpcScanReport(pMsg);
      break;

    case DM_CONN_OPEN_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_CONN_OPEN_IND\r\n");
#endif
      amdtpcOpen(pMsg);
      break;

    case DM_CONN_CLOSE_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_CONN_CLOSE_IND\r\n");
#endif
      amdtpc_proc_msg(&pMsg->hdr);
      break;

    case DM_SEC_PAIR_CMPL_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_SEC_PAIR_CMPL_IND\r\n");
#endif
      break;

    case DM_SEC_PAIR_FAIL_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_SEC_PAIR_FAIL_IND\r\n");
#endif
      break;

    case DM_SEC_ENCRYPT_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_SEC_ENCRYPT_IND\r\n");
#endif
      break;

    case DM_SEC_ENCRYPT_FAIL_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_SEC_ENCRYPT_FAIL_IND\r\n");
#endif
      break;

    case DM_SEC_AUTH_REQ_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_SEC_AUTH_REQ_IND\r\n");
#endif
      if (pMsg->authReq.oob)
      {
        dmConnId_t connId = (dmConnId_t) pMsg->hdr.param;

        /* TODO: Perform OOB Exchange with the peer. */
        /* TODO: Fill datsOobCfg peerConfirm and peerRandom with value passed out of band */

        if (amdtpcOobCfg != NULL)
        {
          DmSecSetOob(connId, amdtpcOobCfg);
        }

        DmSecAuthRsp(connId, 0, NULL);
      }
      else
      {
        AppHandlePasskey(&pMsg->authReq);
      }
      break;

    case DM_SEC_COMPARE_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_SEC_COMPARE_IND\r\n");
#endif
      AppHandleNumericComparison(&pMsg->cnfInd);
      break;

    case DM_ADV_NEW_ADDR_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_ADV_NEW_ADDR_IND\r\n");
#endif
      break;

    case DM_VENDOR_SPEC_CMD_CMPL_IND:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("DM_VENDOR_SPEC_CMD_CMPL_IND\r\n");
#endif
      {
        #if defined(AM_PART_APOLLO) || defined(AM_PART_APOLLO2)

          uint8_t *param_ptr = &pMsg->vendorSpecCmdCmpl.param[0];

          switch (pMsg->vendorSpecCmdCmpl.opcode)
          {
            case 0xFC20: //read at address
            {
              uint32_t read_value;

              BSTREAM_TO_UINT32(read_value, param_ptr);

              //am_menu_printf("VSC 0x%0x complete status %x param %x",
              pMsg->vendorSpecCmdCmpl.opcode,
              pMsg->hdr.status,
              read_value);
            }
            break;
            default:
              //am_menu_printf("VSC 0x%0x complete status %x",
              pMsg->vendorSpecCmdCmpl.opcode,
              pMsg->hdr.status);
            break;
          }

        #endif
      }
      break;

    default:
#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf(" default\r\n");
#endif
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \fn     AmdtpcHandlerInit
 *
 *  \brief  Application handler init function called during system initialization.
 *
 *  \param  handlerID  WSF handler ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AmdtpcHandlerInit(wsfHandlerId_t handlerId)
{
#ifdef AMDTP_MAIN_DEBUG
  am_menu_printf("AmdtpcHandlerInit");
#endif
  /* store handler ID */
  amdtpcCb.handlerId = handlerId;

  /* set handle list length */
  amdtpcCb.hdlListLen = AMDTPC_DISC_HDL_LIST_LEN;

  /* Set configuration pointers */
  pAppMasterCfg = (appMasterCfg_t *) &amdtpcMasterCfg;
  pAppSecCfg = (appSecCfg_t *) &amdtpcSecCfg;
  pAppDiscCfg = (appDiscCfg_t *) &amdtpcDiscCfg;
  pAppCfg = (appCfg_t *)&amdtpcAppCfg;
  pSmpCfg = (smpCfg_t *) &amdtpcSmpCfg;

  /* Initialize application framework */
  AppMasterInit();
  AppDiscInit();

  /* Set IRK for the local device */
  DmSecSetLocalIrk(localIrk);
  amdtpc_init(handlerId, amdtpDtpRecvCback, amdtpDtpTransCback);

  measTpTimer.handlerId = handlerId;
  measTpTimer.msg.event = AMDTP_MEAS_TP_TIMER_IND;
}

/*************************************************************************************************/
/*!
 *  \fn     AmdtpcHandler
 *
 *  \brief  WSF event handler for application.
 *
 *  \param  event   WSF event mask.
 *  \param  pMsg    WSF message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AmdtpcHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  if (pMsg != NULL)
  {
#ifdef AMDTP_MAIN_DEBUG
  am_menu_printf("%s got event %d ",__func__, pMsg->event);
#endif

    /* process ATT messages */
    if (pMsg->event <= ATT_CBACK_END)
    {
#ifdef AMDTP_MAIN_DEBUG
     am_menu_printf("process discovery-related ATT messages \r\n");
#endif
      /* process discovery-related ATT messages */
      AppDiscProcAttMsg((attEvt_t *) pMsg);
    }
    /* process DM messages */
    else if (pMsg->event <= DM_CBACK_END)
    {
#ifdef AMDTP_MAIN_DEBUG
    am_menu_printf("process advertising and connection-related messages \r\n");
#endif
      /* process advertising and connection-related messages */
      AppMasterProcDmMsg((dmEvt_t *) pMsg);

#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("process security-related messages\r\n");
#endif
      /* process security-related messages */
      AppMasterSecProcDmMsg((dmEvt_t *) pMsg);

#ifdef AMDTP_MAIN_DEBUG
      am_menu_printf("process discovery-related message (DM)\r\n");
#endif
      /* process discovery-related messages */
      AppDiscProcDmMsg((dmEvt_t *) pMsg);
    }

#ifdef AMDTP_MAIN_DEBUG
    am_menu_printf("perform profile and user interface-related operations\r\n");
#endif
    /* perform profile and user interface-related operations */
    amdtpcProcMsg((dmEvt_t *) pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \fn     DatcStart
 *
 *  \brief  Start the application.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AmdtpcStart(void)
{
#ifdef AMDTP_MAIN_DEBUG
   am_menu_printf("%s: Register Stack Callbacks...\r\n", __func__);
#endif
  /* Register for stack callbacks */
  DmRegister(amdtpcDmCback);
  DmConnRegister(DM_CLIENT_ID_APP, amdtpcDmCback);
  AttRegister(amdtpcAttCback);

  /* Register for app framework discovery callbacks */
  AppDiscRegister(amdtpcDiscCback);

  /* Initialize attribute server database */
  SvcCoreAddGroup();

  /* Reset the device */
  DmDevReset();
}

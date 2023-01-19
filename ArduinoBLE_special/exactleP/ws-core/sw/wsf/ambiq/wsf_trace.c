/*************************************************************************************************/
/*!
 *  \file   wsf_trace.c
 *
 *  \brief  Trace message implementation.
 *
 *          $Date: 2014-08-06 15:13:15 -0700 (Wed, 06 Aug 2014) $
 *          $Revision: 1708 $
 *
 *  Copyright (c) 2009 Wicentric, Inc., all rights reserved.
 *  Wicentric confidential and proprietary.
 *
 *  IMPORTANT.  Your use of this file is governed by a Software License Agreement
 *  ("Agreement") that must be accepted in order to download or otherwise receive a
 *  copy of this file.  You may not use or copy this file for any purpose other than
 *  as described in the Agreement.  If you do not agree to all of the terms of the
 *  Agreement do not use this file and delete all copies in your possession or control;
 *  if you do not have a copy of the Agreement, you must contact Wicentric, Inc. prior
 *  to any use, copying or further distribution of this software.
 */
/*************************************************************************************************/

#include "wsf_types.h"
#include "wsf_trace.h"
#include "wsf_assert.h"
#include "wsf_cs.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "am_util_debug.h"
#include "am_util_stdio.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

#ifndef WSF_RING_BUF_SIZE
/*! \brief      Size of token ring buffer (multiple of 2^N). */
#define WSF_RING_BUF_SIZE               32
#endif

/*! \brief      Ring buffer flow control condition detected. */
#define WSF_TOKEN_FLAG_FLOW_CTRL        (1 << 28)

/**************************************************************************************************
  Data types
**************************************************************************************************/

#if WSF_TOKEN_ENABLED

/*! \brief      Trace control block. */
struct
{
  struct
  {
    uint32_t token;             /*!< Token. */
    uint32_t param;             /*!< Parameter. */
  } ringBuf[WSF_RING_BUF_SIZE]; /*!< Tokenized tracing ring buffer. */

  volatile uint32_t prodIdx;    /*!< Ring buffer producer index. */
  volatile uint32_t consIdx;    /*!< Ring buffer consumer index. */

  WsfTokenHandler_t pendCback;  /*!< Pending event handler. */

  bool_t ringBufEmpty;          /*!< Ring buffer state. */
} wsfTraceCb;

#endif

#if WSF_TRACE_ENABLED == TRUE

/*************************************************************************************************/
/*!
 *  \brief  Register trace handler.
 *
 *  \param  traceCback    Token event handler.
 *
 *  \return None.
 *
 *  This routine registers trace output handler. This callback is called when the trace data is
 *  ready for writing.
 *
 *  added paulvha
 */
/*************************************************************************************************/
WsfTraceHandler_t sendMsgCback;
bool_t enable_trace = FALSE;
/* Use heap memory to ease stack utilization. */
static char buf_back[200];
static char buf_tmp[10];

void WsfTraceRegisterHandler(WsfTraceHandler_t traceCback)
{
  sendMsgCback =  traceCback;
}

/*************************************************************************************************/
/*!
 *  \fn     WsfTrace
 *
 *  \brief  Print a trace message.
 *
 *  \param  pStr      Message format string
 *  \param  ...       Additional arguments, printf-style
 *
 *  \return None.
 *
 *  changed paulvha to handle better display
 */
/*************************************************************************************************/

void WsfTrace(const char *pStr, ...)
{
  va_list    args;

  // must be enable with WsfTraceEnable()
  if (enable_trace){

    va_start(args, pStr);
    vsnprintf(buf_back, 200, pStr, args);
    va_end(args);

    sendMsgCback((uint8_t *)buf_back, strlen(buf_back));
  }
}

/*************************************************************************************************/
/*!
 *  \fn     WsfTraceEnable
 *
 *  \brief  Enable trace messages.
 *
 *  \param  enable    TRUE to enable, FALSE to disable
 *
 *  \return None.
 *
 *  changed paulvha
 */
/*************************************************************************************************/
void WsfTraceEnable(bool_t enable)
{
   // call back must have been set before with WsfTraceRegisterHandler()
   WSF_ASSERT(sendMsgCback);
   enable_trace = enable;
}

/*************************************************************************************************/
/*!
 *  \fn     WsfPacketTrace
 *
 *  \brief  Print raw HCI data as a trace message.
 *
 *  \param  ui8Type      HCI packet type byte
 *  \param  ui32Len      Length of the HCI packet
 *  \param  pui8Buf      Pointer to the buffer of HCI data
 *
 *  \return None.
 */
/*************************************************************************************************/
void WsfPacketTrace(uint8_t ui8Type, uint32_t ui32Len, uint8_t *pui8Buf)
{
  uint32_t i, j;
  uint8_t lt, lb;

  lb = sprintf(buf_back,"%02X ", ui8Type);          //am_util_debug_printf("%02X ", ui8Type);

  for(i = 0; i < ui32Len; i++)
  {
    if ((i % 8) == 0)
    {

      buf_back[lb++] = '\n';                        //am_util_debug_printf("\n");
    }

    lt = sprintf(buf_tmp,"%02X ", *pui8Buf++);      //am_util_debug_printf("%02X ", *pui8Buf++);
    for (j = 0; j < lt ; j++) buf_back[lb++] = buf_tmp[j];

  }

  //am_util_debug_printf("\n\n");
  buf_back[lb++] = '\n';
  buf_back[lb++] = '\n';
  buf_back[lb] = 0x0;

  sendMsgCback((uint8_t *)buf_back, strlen(buf_back));
}

#elif WSF_TOKEN_ENABLED == TRUE

/*************************************************************************************************/
/*!
 *  \fn     WsfToken
 *
 *  \brief  Output tokenized message.
 *
 *  \param  tok       Token
 *  \param  var       Variable
 *
 *  \return None.
 */
/*************************************************************************************************/
void WsfToken(uint32_t tok, uint32_t var)
{
  static uint32_t flags = 0;

  WSF_CS_INIT(cs);
  WSF_CS_ENTER(cs);

  uint32_t prodIdx = (wsfTraceCb.prodIdx + 1) & (WSF_RING_BUF_SIZE - 1);

  if (prodIdx != wsfTraceCb.consIdx)
  {
    wsfTraceCb.ringBuf[wsfTraceCb.prodIdx].token = tok | flags;
    wsfTraceCb.ringBuf[wsfTraceCb.prodIdx].param = var;
    wsfTraceCb.prodIdx = prodIdx;
    flags = 0;
  }
  else
  {
    flags = WSF_TOKEN_FLAG_FLOW_CTRL;
  }

  WSF_CS_EXIT(cs);

  if (wsfTraceCb.pendCback && wsfTraceCb.ringBufEmpty)
  {
    wsfTraceCb.ringBufEmpty = FALSE;
    wsfTraceCb.pendCback();
  }
}

/*************************************************************************************************/
/*!
 *  \fn     wsfTokenService
 *
 *  \brief  Service the trace ring buffer.
 *
 *  \return TRUE if trace messages pending, FALSE otherwise.
 *
 *  This routine is called in the main loop for a "push" type trace systems.
 */
/*************************************************************************************************/
bool_t WsfTokenService(void)
{
  static uint8_t outBuf[sizeof(wsfTraceCb.ringBuf[0])];
  static uint8_t outBufIdx = sizeof(wsfTraceCb.ringBuf[0]);

  if (outBufIdx < sizeof(wsfTraceCb.ringBuf[0]))
  {
    outBufIdx += WsfTokenIOWrite(outBuf + outBufIdx, sizeof(wsfTraceCb.ringBuf[0]) - outBufIdx);

    if (outBufIdx < sizeof(wsfTraceCb.ringBuf[0]))
    {
      /* I/O device is flow controlled. */
      return TRUE;
    }
  }

  if (wsfTraceCb.consIdx != wsfTraceCb.prodIdx)
  {
    memcpy(&outBuf, &wsfTraceCb.ringBuf[wsfTraceCb.consIdx], sizeof(wsfTraceCb.ringBuf[0]));
    outBufIdx = 0;

    wsfTraceCb.consIdx = (wsfTraceCb.consIdx + 1) & (WSF_RING_BUF_SIZE - 1);

    return TRUE;
  }

  return FALSE;
}

#endif

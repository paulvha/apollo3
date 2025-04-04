/*************************************************************************************************/
/*!
 *  \file   wsf_trace.c
 *
 *  \brief  Trace message implementation.
 *
 *          $Date: 2016-12-28 16:12:14 -0600 (Wed, 28 Dec 2016) $
 *          $Revision: 10805 $
 *
 *  Copyright (c) 2009-2017 ARM Ltd., all rights reserved.
 *  ARM Ltd. confidential and proprietary.
 *
 *  IMPORTANT.  Your use of this file is governed by a Software License Agreement
 *  ("Agreement") that must be accepted in order to download or otherwise receive a
 *  copy of this file.  You may not use or copy this file for any purpose other than
 *  as described in the Agreement.  If you do not agree to all of the terms of the
 *  Agreement do not use this file and delete all copies in your possession or control;
 *  if you do not have a copy of the Agreement, you must contact ARM Ltd. prior
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

  bool_t enabled;               /*!< Tracing state. */
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

  /* Use heap memory to ease stack utilization. */
  static char buf[200];

  // must be enable with WsfTraceEnable()
  if (enable_trace){

    va_start(args, pStr);
    vsnprintf(buf, 200, pStr, args);
    va_end(args);

    sendMsgCback((uint8_t *)buf, strlen(buf));
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
   // cal back must have been set before with WsfTraceRegisterHandler()
   WSF_ASSERT(sendMsgCback);
   enable_trace = enable;
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

  if (!wsfTraceCb.enabled)
  {
    return;
  }

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

/*************************************************************************************************/
/*!
 *  \fn     WsfTraceEnable
 *
 *  \brief  Enable trace messages.
 *
 *  \param  enable    TRUE to enable, FALSE to disable
 *
 *  \return None.
 */
/*************************************************************************************************/
void WsfTraceEnable(bool_t enable)
{
  wsfTraceCb.enabled = enable;
}

#endif

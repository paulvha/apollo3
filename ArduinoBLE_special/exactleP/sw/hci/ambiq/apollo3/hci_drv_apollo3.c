//*****************************************************************************
//
//! @file hci_drv_apollo3.c
//!
//! @brief HCI driver interface.
//
//*****************************************************************************

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
//  special port for ArduinoBLE  January 2021 / paulvha
//
//  Stripped out much of the unnecessary stuff / February  2023 /paulvha
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>

#include "wsf_types.h"
#include "wsf_timer.h"
#include "bstream.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "wsf_cs.h"
#include "hci_drv.h"
#include "hci_drv_apollo.h"
#include "am_mcu_apollo.h"
#include "am_util.h"
#include "hci_drv_apollo3.h"

#include <string.h>

//*****************************************************************************
//
// Configurable buffer sizes.
//
//*****************************************************************************
#define NUM_HCI_WRITE_BUFFERS           8
#define HCI_DRV_MAX_TX_PACKET           256
#define HCI_DRV_MAX_RX_PACKET           256

//*****************************************************************************
//
// Configurable error-detection thresholds.
//
//*****************************************************************************
#define HCI_DRV_MAX_XTAL_RETRIES         10

//*****************************************************************************
//
// Structure for holding outgoing HCI packets.
//
//*****************************************************************************
typedef struct
{
    uint32_t ui32Length;
    uint32_t pui32Data[HCI_DRV_MAX_TX_PACKET / 4];
}
hci_drv_write_t;

//*****************************************************************************
//
// Global variables.
//
//*****************************************************************************

// BLE module handle  (changed to BLE_P due to conflict with BLE in BLELocalDevice.cpp)
void *BLE_P;

// set the BLE MAC address to a special value
uint8_t g_BLEMacAddress[6] = {0x01,0x02,0x03,0x04,0x05,0x06};

// Global handle used to send BLE events about the Hci driver layer.
wsfHandlerId_t g_HciDrvHandleID = 0;

// store call back to ArduinoBLE
data_received_handler_t data_received_handler;

// Buffers for HCI write data.
hci_drv_write_t g_psWriteBuffers[NUM_HCI_WRITE_BUFFERS];
am_hal_queue_t g_sWriteQueue;

// Buffers for HCI read data.
uint32_t g_pui32ReadBuffer[HCI_DRV_MAX_RX_PACKET / 4];
uint8_t *g_pui8ReadBuffer = (uint8_t *) g_pui32ReadBuffer;
uint32_t g_ui32NumBytes   = 0;

void HciDrvEmptyWriteQueue(void);

//*****************************************************************************
//
// Forward declarations for HCI callbacks.
//
//*****************************************************************************
void hciDrvWriteCallback(uint8_t *pui8Data, uint32_t ui32Length, void *pvContext);
void hciDrvReadCallback(uint8_t *pui8Data, uint32_t ui32Length, void *pvContext);

//*****************************************************************************
//
// Events for the HCI driver interface.
//
//*****************************************************************************
#define BLE_TRANSFER_NEEDED_EVENT                   0x01

//*****************************************************************************
//
// Error-handling wrapper macro.
//
//*****************************************************************************
#define ERROR_CHECK_VOID(status)                                              \
    {                                                                         \
        uint32_t ui32ErrChkStatus;                                            \
        if (0 != (ui32ErrChkStatus = (status)))                               \
        {                                                                     \
            am_util_debug_printf("ERROR_CHECK_VOID "#status "\n");            \
            error_check(ui32ErrChkStatus);                                    \
            return;                                                           \
        }                                                                     \
    }

#define ERROR_RETURN(status, retval)                                          \
    if ((status))                                                             \
    {                                                                         \
        error_check(status);                                                  \
        return (retval);                                                      \
    }

#define ERROR_RECOVER(status)                                                 \
    if ((status))                                                             \
    {                                                                         \
        error_check(status);                                                  \
        HciDrvRadioShutdown();                                                \
        HciDrvRadioBoot(0);                                                   \
        HciDrvEmptyWriteQueue();                                              \
        return;                                                               \
    }

//*****************************************************************************
//
// Debug section.
//
//*****************************************************************************
#if 0 // make sure enable in am_util_debug.h
#define CRITICAL_PRINT(...)                                                   \
    do                                                                        \
    {                                                                         \
        AM_CRITICAL_BEGIN;                                                    \
        am_util_debug_printf(__VA_ARGS__);                                    \
        AM_CRITICAL_END;                                                      \
    } while (0)
#else
#define CRITICAL_PRINT(...)
#endif

// for timing pulse
#define AM_DEBUG_BLE_TIMING 1
//*****************************************************************************
//
// Function pointer for redirecting errors
//
//*****************************************************************************
hci_drv_error_handler_t g_hciDrvErrorHandler = 0;
static uint32_t g_ui32FailingStatus = 0;

//*****************************************************************************
//
// By default, errors will be printed. If there is an error handler defined,
// they will be sent there intead.
//
//*****************************************************************************
static void
error_check(uint32_t ui32Status)
{
    //
    // Don't do anything unless there's an error.
    //
    if (ui32Status)
    {
        //
        // Set the global error status. If there's an error handler function,
        // call it. Otherwise, just print the error status and wait.
        //
        g_ui32FailingStatus = ui32Status;

        if (g_hciDrvErrorHandler)
        {
            g_hciDrvErrorHandler(g_ui32FailingStatus);
        }
        else
        {
            CRITICAL_PRINT("Error detected: 0x%08x\n", g_ui32FailingStatus);
            CRITICAL_PRINT("BSTATUS: 0x%08x\n", BLEIF->BSTATUS);
        }
    }
}

//*****************************************************************************
//
// Other useful macros.
//
//*****************************************************************************

#define BLE_IRQ_CHECK()             (BLEIF->BSTATUS_b.BLEIRQ)

//*****************************************************************************
//
// Boot the radio.
//
//*****************************************************************************
void
HciDrvRadioBoot(bool bColdBoot)
{
    uint32_t ui32NumXtalRetries = 0;
    g_ui32NumBytes     = 0;

#ifdef AM_DEBUG_BLE_TIMING    // ATP pins
    am_hal_gpio_pinconfig(28, g_AM_HAL_GPIO_OUTPUT);    // interrupt from HCI layer
    am_hal_gpio_pinconfig(27, g_AM_HAL_GPIO_OUTPUT);    // call handler
    am_hal_gpio_pinconfig(23, g_AM_HAL_GPIO_OUTPUT);    // read call back
    am_hal_gpio_pinconfig(22, g_AM_HAL_GPIO_OUTPUT);    // handler
    //am_hal_gpio_pinconfig(4, g_AM_HAL_GPIO_OUTPUT);
#endif // AM_DEBUG_BLE_TIMING

    //
    // Configure and enable the BLE interface.
    //
    uint32_t ui32Status = AM_HAL_STATUS_FAIL;
    while (ui32Status != AM_HAL_STATUS_SUCCESS)
    {
        ERROR_CHECK_VOID(am_hal_ble_initialize(0, &BLE_P));
        ERROR_CHECK_VOID(am_hal_ble_power_control(BLE_P, AM_HAL_BLE_POWER_ACTIVE));

        am_hal_ble_config_t sBleConfig =
        {
            // Configure the HCI interface clock for 6 MHz
            .ui32SpiClkCfg = AM_HAL_BLE_HCI_CLK_DIV8,

            // Set HCI read and write thresholds to 32 bytes each.
            .ui32ReadThreshold = 32,
            .ui32WriteThreshold = 32,

            // The MCU will supply the clock to the BLE core.
            .ui32BleClockConfig = AM_HAL_BLE_CORE_MCU_CLK,

            // Default settings for expected BLE clock drift (measured in PPM).
            .ui32ClockDrift = 0,
            .ui32SleepClockDrift = 50,

            // Default setting - AGC Enabled
            .bAgcEnabled = true,

            // Default setting - Sleep Algo enabled
            .bSleepEnabled = true,

            // Apply the default patches when am_hal_ble_boot() is called.
            .bUseDefaultPatches = true,
        };

        ERROR_CHECK_VOID(am_hal_ble_config(BLE_P, &sBleConfig));

        //
        // Delay 1s for 32768Hz clock stability. This isn't required unless this is
        // our first run immediately after a power-up.
        //
        if ( bColdBoot )
        {
            am_util_delay_ms(1000);
        }
        //
        // Attempt to boot the radio.
        //
        ui32Status = am_hal_ble_boot(BLE_P);

        //
        // Check our status.
        //
        if (ui32Status == AM_HAL_STATUS_SUCCESS)
        {
            //
            // If the radio is running, we can exit this loop.
            //
            break;
        }
        else if (ui32Status == AM_HAL_BLE_32K_CLOCK_UNSTABLE)
        {
            //
            // If the radio is running, but the clock looks bad, we can try to
            // restart.
            //
            ERROR_CHECK_VOID(am_hal_ble_power_control(BLE_P, AM_HAL_BLE_POWER_OFF));
            ERROR_CHECK_VOID(am_hal_ble_deinitialize(BLE_P));

            //
            // We won't restart forever. After we hit the maximum number of
            // retries, we'll just return with failure.
            //
            if (ui32NumXtalRetries++ < HCI_DRV_MAX_XTAL_RETRIES)
            {
                am_util_delay_ms(1000);
            }
            else
            {
                return;
            }
        }
        else
        {
            ERROR_CHECK_VOID(am_hal_ble_power_control(BLE_P, AM_HAL_BLE_POWER_OFF));
            ERROR_CHECK_VOID(am_hal_ble_deinitialize(BLE_P));

            //
            // If the radio failed for some reason other than 32K Clock
            // instability, we should just report the failure and return.
            //
            error_check(ui32Status);
            return;
        }
    }

    //
    // Set the BLE TX Output power to 0dBm.
    //
    am_hal_ble_tx_power_set(BLE_P, 0x8);

    //
    // Enable interrupts for the BLE module.
    //

 //   am_hal_ble_int_clear(BLE_P, (AM_HAL_BLE_INT_CMDCMP |
 //                              AM_HAL_BLE_INT_DCMP |
 //                              AM_HAL_BLE_INT_BLECIRQ |
 //                              AM_HAL_BLE_INT_BLECSSTAT));
    am_hal_ble_int_clear(BLE_P,0xffff);   // clear all

    am_hal_ble_int_enable(BLE_P, (AM_HAL_BLE_INT_CMDCMP |   // Command Complete interrupt
                                AM_HAL_BLE_INT_DCMP |       // DMA Complete.
                                AM_HAL_BLE_INT_BLECIRQ |    // availability of read data from the BLE Core
                                AM_HAL_BLE_INT_BLECSSTAT)); // writes can be done

    if (APOLLO3_GE_B0)
    {
        am_hal_ble_int_clear(BLE_P, (AM_HAL_BLE_INT_BLECIRQN |
                                   AM_HAL_BLE_INT_BLECSSTATN));

        am_hal_ble_int_enable(BLE_P, (AM_HAL_BLE_INT_BLECIRQN |
                                    AM_HAL_BLE_INT_BLECSSTATN));
    }

   // CRITICAL_PRINT("INTEN:  %d\n", BLEIF->INTEN_b.BLECSSTAT);
   // CRITICAL_PRINT("INTENREG:  %d\n", BLEIF->INTEN);


    NVIC_EnableIRQ(BLE_IRQn);

    //
    // Initialize a queue to help us keep track of HCI write buffers.
    //
    am_hal_queue_from_array(&g_sWriteQueue, g_psWriteBuffers);

    return;
}

//*****************************************************************************
//
// Shut down the BLE core.
//
//*****************************************************************************
void
HciDrvRadioShutdown(void)
{
    NVIC_DisableIRQ(BLE_IRQn);

    ERROR_CHECK_VOID(am_hal_ble_power_control(BLE_P, AM_HAL_BLE_POWER_OFF));

    while ( PWRCTRL->DEVPWREN_b.PWRBLEL );

    ERROR_CHECK_VOID(am_hal_ble_deinitialize(BLE_P));
}


//*****************************************************************************
//
// set call back to ArduinoBLE to store the read data
//
//*****************************************************************************
void
HciDrvset_data_received_handler(data_received_handler_t handler)
{
    data_received_handler = handler;
}

//*****************************************************************************
//
// update BLE core to start sending data
//
//*****************************************************************************
static void
update_wake(void)
{
    AM_CRITICAL_BEGIN;

    //
    // We want to set WAKE if there's something in the write queue, but not if
    // SPISTATUS or IRQ is high.
    //
    if ( !am_hal_queue_empty(&g_sWriteQueue) &&
         (BLEIFn(0)->BSTATUS_b.SPISTATUS == 0) &&
         (BLE_IRQ_CHECK() == false))
    {
        am_hal_ble_wakeup_set(BLE_P, 1);

        //
        // If we've set wakeup, but IRQ came up at the same time, we should
        // just lower WAKE again.
        //
        if (BLE_IRQ_CHECK() == true)
        {
            am_hal_ble_wakeup_set(BLE_P, 0);
        }
    }

    AM_CRITICAL_END;
}

//*****************************************************************************
//
// Function used by the BLE stack to send HCI messages to the BLE controller.
//
// Internally, the Cordio BLE stack will allocate memory for an HCI message,
//
//*****************************************************************************
uint16_t
hciDrvWrite(uint8_t type, uint16_t len, uint8_t *pData)
{
    uint8_t *pui8Wptr;
    hci_drv_write_t *psWriteBuffer;

    //
    // Check to see if we still have buffer space.
    //
    if (am_hal_queue_full(&g_sWriteQueue))
    {
        CRITICAL_PRINT("ERROR: Ran out of HCI transmit queue slots.\n");
        ERROR_RETURN(HCI_DRV_TRANSMIT_QUEUE_FULL, len);
    }

    if (len > (HCI_DRV_MAX_TX_PACKET-1))  // comparison compensates for the type byte at index 0.
    {
        CRITICAL_PRINT("ERROR: Trying to send an HCI packet larger than the hci driver buffer size (needs %d bytes of space).\n",
                       len);

        ERROR_RETURN(HCI_DRV_TX_PACKET_TOO_LARGE, len);
    }

    //
    // Get a pointer to the next item in the queue.
    //
    psWriteBuffer = am_hal_queue_next_slot(&g_sWriteQueue);

    //
    // Set all of the fields in the hci write structure.
    //
    psWriteBuffer->ui32Length = len + 1;

    pui8Wptr = (uint8_t *) psWriteBuffer->pui32Data;

    *pui8Wptr++ = type;

    for (uint32_t i = 0; i < len; i++)
    {
        pui8Wptr[i] = pData[i];
    }

    am_hal_queue_item_add(&g_sWriteQueue, 0, 1);

    //
    // Wake up the BLE controller.
    //
    CRITICAL_PRINT("INFO: HCI write requested.\n");

    update_wake();

#ifdef AM_CUSTOM_BDADDR   // defined during compile
    if (type == HCI_CMD_TYPE)
    {
        uint16_t opcode;
        BYTES_TO_UINT16(opcode, pData);

        if (HCI_OPCODE_RESET == opcode)
        {
            extern uint8_t g_BLEMacAddress[6];
            am_hal_mcuctrl_device_t sDevice;
            am_hal_mcuctrl_info_get(AM_HAL_MCUCTRL_INFO_DEVICEID, &sDevice);
            g_BLEMacAddress[0] = sDevice.ui32ChipID0;
            g_BLEMacAddress[1] = sDevice.ui32ChipID0 >> 8;
            g_BLEMacAddress[2] = sDevice.ui32ChipID0 >> 16;

            HciVendorSpecificCmd(0xFC32, 6, g_BLEMacAddress);
        }
    }
#endif

    return len;
}

void   // called from ArduinoBLE
HciDrvHandlerInit(wsfHandlerId_t handlerId)
{
    g_HciDrvHandleID = handlerId;

    HciDrvEmptyWriteQueue();
}

//*****************************************************************************
//
// Simple interrupt handler to call
//
// Note: These two lines need to be added to the exactle initialization
// function at the beginning of all Cordio applications:
//
//     handlerId = WsfOsSetNextHandler(HciDrvHandler);
//     HciDrvHandler(handlerId);
//
//*****************************************************************************
void
HciDrvIntService(void)
{
#if AM_DEBUG_BLE_TIMING
    am_hal_gpio_state_write(28, AM_HAL_GPIO_OUTPUT_SET);
#endif

    //
    // Read and clear the interrupt status.
    //
    uint32_t ui32Status = am_hal_ble_int_status(BLE_P, true);
    am_hal_ble_int_clear(BLE_P, ui32Status);

    //CRITICAL_PRINT("INFO: IRQ status %x\n", ui32Status);

    //
    // Handle any DMA or Command Complete interrupts.
    //
    am_hal_ble_int_service(BLE_P, ui32Status);

    //
    // If this was a BLEIRQ interrupt, attempt to start a read operation. If it
    // was a STATUS interrupt, start a write operation.
    //
    if (ui32Status & AM_HAL_BLE_INT_BLECIRQ)
    {
       // CRITICAL_PRINT("INFO: IRQ INTERRUPT\n");
#if AM_DEBUG_BLE_TIMING
    am_hal_gpio_state_write(27, AM_HAL_GPIO_OUTPUT_SET);
#endif
        //
        // Lower WAKE
        //
        //CRITICAL_PRINT("BLECIRQ");
        am_hal_ble_wakeup_set(BLE_P, 0);

        //
        // Prepare to read a message.
        //
        WsfSetEvent(g_HciDrvHandleID, BLE_TRANSFER_NEEDED_EVENT);

#if AM_DEBUG_BLE_TIMING
    am_hal_gpio_state_write(27, AM_HAL_GPIO_OUTPUT_CLEAR);
#endif
    }
    else if (ui32Status & AM_HAL_BLE_INT_BLECSSTAT)
    {
        //CRITICAL_PRINT("INFO: STATUS INTERRUPT\n");

        //
        // Check the queue and send the first message we have.
        //
        if ( !am_hal_queue_empty(&g_sWriteQueue) )
        {
            uint32_t ui32WriteStatus = 0;

            hci_drv_write_t *psWriteBuffer = am_hal_queue_peek(&g_sWriteQueue);

            ui32WriteStatus =
                am_hal_ble_nonblocking_hci_write(BLE_P,
                                                 AM_HAL_BLE_RAW,
                                                 psWriteBuffer->pui32Data,
                                                 psWriteBuffer->ui32Length,
                                                 hciDrvWriteCallback,
                                                 0);

            //
            // If it succeeded, we can pop the queue.
            //
            if (ui32WriteStatus == AM_HAL_STATUS_SUCCESS)
            {
                //CRITICAL_PRINT("INFO: HCI write sent.\n");
            }
            else
            {
                //CRITICAL_PRINT("INFO: HCI write failed: %d\n", ui32WriteStatus);
            }
        }
    }

#if AM_DEBUG_BLE_TIMING
    am_hal_gpio_state_write(28, AM_HAL_GPIO_OUTPUT_CLEAR);
#endif
}

//*****************************************************************************
//
// This function determines what to do when a write operation completes.
//
//*****************************************************************************
void  // USE_NONBLOCKING_HCI
hciDrvWriteCallback(uint8_t *pui8Data, uint32_t ui32Length, void *pvContext)
{
    //CRITICAL_PRINT("INFO: HCI physical write complete.\n");

    //
    // Advance the queue.
    //
    am_hal_queue_item_get(&g_sWriteQueue, 0, 1);

    while ( BLEIFn(0)->BSTATUS_b.SPISTATUS )
    {
        am_util_delay_us(5);
    }

    //
    // Check the write queue, and possibly set wake again.
    //
    if ( !am_hal_queue_empty(&g_sWriteQueue) )
    {
        //
        // In this case, we need to delay before setting wake. Instead of
        // delaying here, we'll post an event to do it later.
        //
        WsfSetEvent(g_HciDrvHandleID, BLE_TRANSFER_NEEDED_EVENT);
    }
}

//*****************************************************************************
//
// This function determines what to do when a read operation completes.
//
//*****************************************************************************
void // USE_NONBLOCKING_HCI
hciDrvReadCallback(uint8_t *pui8Data, uint32_t ui32Length, void *pvContext)
{
#if AM_DEBUG_BLE_TIMING
    am_hal_gpio_state_write(23, AM_HAL_GPIO_OUTPUT_SET);
#endif
    //
    // Set a "transfer needed" event.
    //
    // CRITICAL_PRINT("INFO: HCI physical read complete.\n");
    g_ui32NumBytes = ui32Length;
    WsfSetEvent(g_HciDrvHandleID, BLE_TRANSFER_NEEDED_EVENT);

    //
    // wait to drop interrupt
    //
    while ( BLE_IRQ_CHECK() )
    {
        am_util_delay_us(5);
    }

    //
    // Check the write queue, and possibly set wake.
    //
    if ( !am_hal_queue_empty(&g_sWriteQueue) )
    {
        am_hal_ble_wakeup_set(BLE_P, 1);
    }
#if AM_DEBUG_BLE_TIMING
    am_hal_gpio_state_write(23, AM_HAL_GPIO_OUTPUT_CLEAR);
#endif
}

//*****************************************************************************
//
// Event handler for HCI-related events.
//
// This handler can perform HCI reads or writes, and keeps the actions in the
// correct order.
// USE_NONBLOCKING_HCI / interrupt driven
//*****************************************************************************
void
HciDrvHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  uint32_t ui32ErrorStatus;
#if AM_DEBUG_BLE_TIMING
    am_hal_gpio_state_write(22, AM_HAL_GPIO_OUTPUT_SET);
#endif
  //
  // Check to see if we read any bytes over the HCI interface that we haven't
  // already sent to the BLE stack.
  //
  if (g_ui32NumBytes > 0)
  {
      CRITICAL_PRINT("INFO: HCI data transferred to stack.\n");

      //
      // Pass the data along to the stack. The stack should be able
      // to read as much data as we send it.  If it can't, we need to
      // know that.
      // assume all bytes can be send
      data_received_handler(g_pui8ReadBuffer, g_ui32NumBytes);

      CRITICAL_PRINT("INFO: HCI RX packet complete.\n");
      g_ui32NumBytes   = 0;
  }

  if ( BLE_IRQ_CHECK() )
  {
      //
      // If the stack has used up all of the saved data we've accumulated so
      // far, we should check to see if we need to read any *new* data.
      // once all the data has been read it will call hciDrvReadCallback(), this will
      // set the correct g_ui32NumBytes and call this handler again. Once called
      // the read data is passed to the upper layer
      //
      CRITICAL_PRINT("INFO: HCI Read started.\n");
      ui32ErrorStatus = am_hal_ble_nonblocking_hci_read(BLE_P,
                                                        g_pui32ReadBuffer,
                                                        hciDrvReadCallback,
                                                        0);

      if (g_ui32NumBytes > HCI_DRV_MAX_RX_PACKET)
      {
          CRITICAL_PRINT("ERROR: Trying to receive an HCI packet "
                         "larger than the hci driver buffer size "
                         "(needs %d bytes of space).\n",
                         g_ui32NumBytes);
#if AM_DEBUG_BLE_TIMING
    am_hal_gpio_state_write(22, AM_HAL_GPIO_OUTPUT_CLEAR);
#endif
          ERROR_CHECK_VOID(HCI_DRV_RX_PACKET_TOO_LARGE);
      }

      if (ui32ErrorStatus != AM_HAL_STATUS_SUCCESS)
      {
          //
          // If the read didn't succeed for some physical reason, we need
          // to know. We shouldn't get failures here. We checked the IRQ
          // signal before calling the read function, and this driver
          // only contains a single call to the blocking read function,
          // so there shouldn't be any physical reason for the read to
          // fail.
          //
          CRITICAL_PRINT("HCI READ failed with status %d. "
                         "Try recording with a logic analyzer "
                         "to catch the error.\n",
                         ui32ErrorStatus);
#if AM_DEBUG_BLE_TIMING
    am_hal_gpio_state_write(22, AM_HAL_GPIO_OUTPUT_CLEAR);
#endif
          ERROR_RECOVER(ui32ErrorStatus);
      }
  }
#if AM_DEBUG_BLE_TIMING
    am_hal_gpio_state_write(22, AM_HAL_GPIO_OUTPUT_CLEAR);
#endif
}

//*****************************************************************************
//
// Register an error handler for the HCI driver.
//
//*****************************************************************************
void
HciDrvErrorHandlerSet(hci_drv_error_handler_t pfnErrorHandler)
{
    g_hciDrvErrorHandler = pfnErrorHandler;
}

/*************************************************************************************************/
/*!
 *  \fn     HciVsA3_SetRfPowerLevelEx
 *
 *  \brief  Vendor specific command for settting Radio transmit power level
 *          for Nationz.
 *
 *  \param  txPowerlevel    valid range from 0 to 15 in decimal.
 *
 *  \return true when success, otherwise false
 */
/*************************************************************************************************/
bool_t
HciVsA3_SetRfPowerLevelEx(txPowerLevel_t txPowerlevel)
{
    switch (txPowerlevel) {

        case TX_POWER_LEVEL_MINUS_10P0_dBm:
            am_hal_ble_tx_power_set(BLE_P,0x04);
            return true;
            break;
        case TX_POWER_LEVEL_0P0_dBm:
            am_hal_ble_tx_power_set(BLE_P,0x08);
            return true;
            break;
        case TX_POWER_LEVEL_PLUS_3P0_dBm:
            am_hal_ble_tx_power_set(BLE_P,0x0F);
            return true;
            break;
        default:
            return false;
            break;
    }
}

/*********************************************************************/
/*!
 *  \fn     HciVsA3_ConstantTransmission
 *
 *  \brief  This procedure is to enable/disable BLE Radio into constant transmission mode.
 *
 *  \param  start  BLE controller enters constant transmission mode if true
 *
 *  \return true when success, otherwise false
 */
/*********************************************************************/
void
HciVsA3_ConstantTransmission(uint8_t txchannel)
{
    am_util_ble_set_constant_transmission_ex(BLE_P, txchannel);
}

/*********************************************************************/
/*!
 *  \fn     HciVsA3_SetConstantTransmission
 *
 *  \brief  This procedure is to start/stop carrier wave output in BLE Radio.
 *
 *  \param  start  BLE controller enters carrier wave output mode if true
 *
 *  \return true when success, otherwise false
 */
/*********************************************************************/
void
HciVsA3_CarrierWaveMode(uint8_t txchannel)
{
    am_util_ble_transmitter_control_ex(BLE_P, txchannel);
}

/*********************************************************************/
/*!
 *  \fn     HciDrvBleSleepSet
 *
 *  \brief  Set BLE sleep enable/disable for the BLE core.
 *
 *  \param  enable 'true' set sleep enable, 'false' set sleep disable
 *
 *  \return none
 */
/*********************************************************************/
void
HciDrvBleSleepSet(bool enable)
{
    am_hal_ble_sleep_set(BLE_P, enable);
}

//*****************************************************************************
//
// Clear the HCI write queue
//
//*****************************************************************************
void
HciDrvEmptyWriteQueue(void)
{
    am_hal_queue_from_array(&g_sWriteQueue, g_psWriteBuffers);
}

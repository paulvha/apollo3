/*
 * Copyright (c) Paul van Haastrecht
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file can be used to enable tracing on BLE in Mbed
 *
 * developed by paulvha / version 1.0 / October 2021
 *
 * See BOTTOM OF THIS FILE FOR INSTRUCTIONS
 */

#include "wsf_types.h"
#include "wsf_trace.h"

#ifdef BLE_Debug
#warning "make sure to enable tracing in Mbed. see SetDebugBLE.txt"

bool MBED_BLE_Debug = false;
void traceCback(uint8_t *pBuf, long unsigned int len);
////////////////////////////////////////////////////////////////////////
//   MBED_BLE trace only
//////////////////////////////////////////////////////////////////////
void Enable_BLE_Debug(bool act)
{
  MBED_BLE_Debug = act;
  WsfTraceRegisterHandler(( WsfTraceHandler_t) traceCback);
  WsfTraceEnable(true);
}

void traceCback(uint8_t *pBuf, long unsigned int len)
{
  // if not enabled
  if (! MBED_BLE_Debug) return;

  Serial.print("traceCback: ");

  for (uint32_t i = 0; i < len; i++)
  {
    if (pBuf[i] > 0x1f) Serial.print((char) pBuf[i]);
    else Serial.print(".");
  }
  Serial.println("\r");
}

#endif

/**
 * ========================================================================
 * INSTRUCTIONS for CORDIO stack tracing
 * =========================================================================
 *
 *
 * CHANGES TO MBED
 * ===============
 *
 * copy the complete mbed-os directory from arduino15/packages/SparkFun/hardware/apollo3/2.x.x/cores to
 * another location.(I used Downloads/bm/)
 *
 * In the file
 * mbed-os/features/FEATURE_BLE/targets/TARGET_CORDIO/stack/wsf/include/wsftrace.h
 *
 * Add in the top  #define WSF_TRACE_ENABLED   TRUE  //enable debuggging
 *
 * like :
     /**************************************************************************************************
       Macros
      ************************************************************************************************** /
      #define WSF_TRACE_ENABLED   TRUE  //enable debuggging
 *
 * In the different BLE ccp, header and ttp files you can now add print statements like :
     WsfTrace("In %s, %s\r\n",__FILE__, __func__);  // this will print the filename and routine
     WsfTrace("In variable %x\r\n",variable);       // this will print the variable
 *
 * In some of these files you need to add a reference to wsftrace. If compile fails then include the header files
    #include "wsf_types.h"
    #include "wsf_trace.h"
 *
 * Once the changes have been made you need to recompile and recreate the lib. Make sure to select the right
 * variant. In this case SFE_ARTEMIS_ATP
 *
 * mbed compile --library --source=mbed-os  -m SFE_ARTEMIS_ATP -t GCC_ARM  -D AM_CUSTOM_BDADDR -D ARDUINO
 *
 * Now you need to copy the new libmbed-os.a with mbed from
 *
 * Downloads/bm/BUILD/libraries/bm/SFE_ARTEMIS_ATP/GCC_ARM
 *
 * arduino15/packages/SparkFun/hardware/apollo3/2.X.X/variants/SFE_ARTEMIS_ATP/mbed
 *
 * ***************** AGAIN USE THE RIGHT VARIANT NAME !! ************************************
 *
 * CHANGE IN THE SKETCH
 * ====================
 * add in header sketch :

      #include "ble_debug.h"
      //#define BLE_Debug 1   //uncomment enable printing CORDIO trace messages

 * add in setup():

     #ifdef BLE_Debug
        EnableCordioDebug(true);
        WsfTraceRegisterHandler(( WsfTraceHandler_t) traceCback);
        WsfTraceEnable(true);
      #endif
 *
 * If all done right you now need to recompile and upload the sketch. Once restarted you should see
 * debug messages.
 *
 * ==================================================================================================
 * HCI MESSAGE TRACING
 * ==================================================================================================
 *
 * copy the complete mbed-os directory from arduino15/packages/SparkFun/hardware/apollo3/2.X.X/cores to
 * another location.(I used Downloads/bm/)
 *
 * In the file
 * mbed-os/features/FEATURE_BLE/targets/TARGET_Ambiq_Micro/TARGET_Apollo3/AP3CordioHCITransportDriver.cpp
 *
 * in top change
 *
    #if PRINT_DEBUG_HCI
    #include "mbed.h"
    DigitalOut debugGPIO(D28, 0);
    DigitalOut debugGPIO2(D25, 0);
    #endif
 * to

    #define PRINT_DEBUG_HCI 1   // enable HCI messages

    // commented out as we do not want pulses
    //  #if PRINT_DEBUG_HCI
    //  #include "mbed.h"
    //  DigitalOut debugGPIO(D28, 0);
    //  DigitalOut debugGPIO2(D25, 0);
    //  #endif

 * Once the changes have been made you need to recompile and recreate the lib. Make sure to select the right
 * variant. In this case SFE_ARTEMIS_ATP
 *
 * mbed compile --library --source=mbed-os  -m SFE_ARTEMIS_ATP -t GCC_ARM  -D AM_CUSTOM_BDADDR -D ARDUINO
 *
 * Now you need to copy the new libmbed-os.a from
 *
 * Downloads/bm/BUILD/libraries/bm/SFE_ARTEMIS_ATP/GCC_ARM
 *
 * arduino15/packages/SparkFun/hardware/apollo3/2.X.X/variants/SFE_ARTEMIS_ATP/mbed
 *
 * ***************** AGAIN USE THE RIGHT VARIANT NAME !! ************************************
 *
 * If all done right you now need to recompile and upload the sketch. once restarted you should see
 * HCI messages.
*/

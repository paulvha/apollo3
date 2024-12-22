/*
 *  file for apollo3 GPIO control
 *
 *  @brief Functions for accessing and configuring the GPIO module.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  For detailed explanation see included README.md
 *
 *  Version 1.0 / May 2021 / Paulvha
 */

#include "APgpio.h"

// valid for all pads
am_hal_gpio_pincfg_t PadDefOut =
{
  .uFuncSel = 3,                                          // set pin as GPIO
  .ePowerSw = AM_HAL_GPIO_PIN_POWERSW_NONE,               // no power switch
  .ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE,                 // no pullup
  .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,   // 12mA max output
  .eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,           // A push-pull GPIO has the ability to both source and sink current
  .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
  .eIntDir = 0x0,                                         // NO interrupts
  .eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read read pad value
};

// Input valid for all pads
am_hal_gpio_pincfg_t PadDefInput =
{
  .uFuncSel = 3,                                          // set pin as GPIO
  .ePowerSw = AM_HAL_GPIO_PIN_POWERSW_NONE,               // no power switch
  .ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE,                 // No pullup
  .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,    // ignored (valid for output only)
  .eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_DISABLE,            // No output
  .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
  .eIntDir = 0x0,                                         // NO interrupts & enable reading pin
  .eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read the pad
};

// Pullup only valid for pads 0, 1, 5, 6, 8, 9, 25, 27, 39, 40, 42, 43, 48 and 49.
// PullDown only valid for pad 20
am_hal_gpio_pincfg_t PadDefPull =
{
  .uFuncSel = 3,                                          // set pin as GPIO
  .ePowerSw = AM_HAL_GPIO_PIN_POWERSW_NONE,               // no power switch
  .ePullup =  AM_HAL_GPIO_PIN_PULLUP_12K,                 // 12K pullup
  .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,    // ignored (vald for output only)
  .eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_DISABLE,            // No output
  .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
  .eIntDir = 0x0,                                         // NO interrupts & enable reading pin
  .eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read the pad
};

// Output only valid for Pad 3 and 36
am_hal_gpio_pincfg_t PadDefVDD =
{
  .uFuncSel = 3,                                          // set pin as GPIO
  .ePowerSw = AM_HAL_GPIO_PIN_POWERSW_VDD,                // power switch to 3v3
  .ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE,                 // no pullup
  .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,    // weak
  .eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,           // A push-pull GPIO has the ability to both source and sink current
  .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
  .eIntDir = 0x0,                                         // NO interrupts
  .eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read.. read as zeros
};

// Output only valid for Pad 37 and 41
am_hal_gpio_pincfg_t PadDefVSS =
{
  .uFuncSel = 3,                                          // set pin as GPIO
  .ePowerSw = AM_HAL_GPIO_PIN_POWERSW_VSS,                // power switch GND
  .ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE,                 // no pullup
  .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,    // weak
  .eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,           // A push-pull GPIO has the ability to both source and sink current
  .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
  .eIntDir = 0x0,                                         // NO interrupts
  .eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read.. read as zeros
};

//*****************************************************************************
//
//  g_ui8Bit76Capabilities[]
//  This lookup table specifies capabilities of each pad for PADREG bits 7:6.
//
//  Source : am_hal_gpio.c // This is part of revision 2.4.2 of the AmbiqSuite Development Package.
//*****************************************************************************
#define CAP_PUP     0x01    // PULLUP
#define CAP_PDN     0x08    // PULLDOWN (pin 20 only)
#define CAP_VDD     0x02    // VDD PWR (power source)
#define CAP_VSS     0x04    // VSS PWR (ground sink)
#define CAP_RSV     0x80    // bits 7:6 are reserved for this pin
static const uint8_t
g_ui8Bit76Capabilities[AM_HAL_GPIO_MAX_PADS] =
{
    //0        1        2        3        4        5        6        7        8        9
    CAP_PUP, CAP_PUP, CAP_RSV, CAP_VDD, CAP_RSV, CAP_PUP, CAP_PUP, CAP_RSV, CAP_PUP, CAP_PUP,   // Pins 0-9
    CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV,   // Pins 10-19
    CAP_PDN, CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV, CAP_PUP, CAP_RSV, CAP_PUP, CAP_RSV, CAP_RSV,   // Pins 20-29
    CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV, CAP_VDD, CAP_VSS, CAP_RSV, CAP_PUP,   // Pins 30-39
    CAP_PUP, CAP_VSS, CAP_PUP, CAP_PUP, CAP_RSV, CAP_RSV, CAP_RSV, CAP_RSV, CAP_PUP, CAP_PUP    // Pins 40-49
};

//*********************************************************************
// INTERNAL ROUTINES
//*********************************************************************

// set pad as input
bool setINPUT(uint32_t pad){

    if (am_hal_gpio_pinconfig(pad, PadDefInput) == AM_HAL_STATUS_SUCCESS)
        return(true);

    return(false);
}

/* set pad as output
 * parameter arg : 2 ,4 ,8 12mA (default is 12mA if not defined)
 */
bool setOUTPUT(uint32_t pad, int arg){

    switch(arg) {
        case D_2:
            PadDefOut.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA;
            break;
        case D_4:
            PadDefOut.eDriveStrength =  AM_HAL_GPIO_PIN_DRIVESTRENGTH_4MA;
            break;
        case D_8:
            PadDefOut.eDriveStrength =  AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA;
            break;
        case D_12:
        default:
            PadDefOut.eDriveStrength =  AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA;
            break;
    }

    if (am_hal_gpio_pinconfig(pad, PadDefOut) == AM_HAL_STATUS_SUCCESS)
        return(true);

    return(false);
}

// set pull down (only pin 20)
bool setPULLDOWN(uint32_t pad) {

    if (g_ui8Bit76Capabilities[pad] & CAP_PDN) {

        PadDefPull.ePullup = AM_HAL_GPIO_PIN_PULLDOWN;

        if (am_hal_gpio_pinconfig(pad, PadDefPull) == AM_HAL_STATUS_SUCCESS)
            return(true);
    }

    return(false);
}

/*
 * set pullup on pad
 * parameter arg : 1, 6, 12, 24 K pullup (default is 24K if not defined)
 */
bool setPULLUP(uint32_t pad, int arg) {

    if (g_ui8Bit76Capabilities[pad] & CAP_PUP) {

        switch(arg) {
            case R_1_5:
                PadDefPull.ePullup =  AM_HAL_GPIO_PIN_PULLUP_1_5K;
                break;
            case R_6:
                PadDefPull.ePullup =  AM_HAL_GPIO_PIN_PULLUP_6K;
                break;
            case R_12:
                PadDefPull.ePullup =  AM_HAL_GPIO_PIN_PULLUP_12K;
                break;
            case R_24:
            default:
                PadDefPull.ePullup =  AM_HAL_GPIO_PIN_PULLUP_24K;
                break;
        }

        if (am_hal_gpio_pinconfig(pad, PadDefPull) == AM_HAL_STATUS_SUCCESS)
            return(true);
    }

    return(false);
}

// set power switch on pad to 3V3
// MAX 100mA
bool setVDD(uint32_t pad){

    if (g_ui8Bit76Capabilities[pad] & CAP_VDD) {

        if (am_hal_gpio_pinconfig(pad, PadDefVDD) == AM_HAL_STATUS_SUCCESS)
            return(true);
    }

    return(false);
}

// set power switch on pad to GND
// MAX 100mA
bool setVSS(uint32_t pad){

    if (g_ui8Bit76Capabilities[pad] & CAP_VSS) {

        if (am_hal_gpio_pinconfig(pad, PadDefVSS) == AM_HAL_STATUS_SUCCESS)
            return(true);
    }

    return(false);
}

//********************************************************************
// EXTERNAL ROUTINES
//********************************************************************

/* set mode
 * parameter pad  : pad number on Apollo3
 * parameter mode : mode to set
 * parameter arg  : optional additional argument
 */
bool APmode(uint32_t pad, Mode mode, int arg)
{
    if (pad > 50) return (false);

    switch(mode){
        case M_VDD:
            return(setVDD(pad));
            break;
        case M_VSS:
            return(setVSS(pad));
            break;
        case M_INPUT:
            return(setINPUT(pad));
            break;
        case M_OUTPUT:
            return(setOUTPUT(pad, arg));
            break;
        case M_PULLUP:
            return(setPULLUP(pad, arg));
            break;
        case M_PULLDOWN:
            return(setPULLDOWN(pad));
            break;
        default:
            return(false);
            break;
    }
}

/* set output value on pin
 * parameter pad : pad number on Apollo3
 * parameter val : 1 = high, 0 = low
 */
bool APset(uint32_t pad, int val){

   uint32_t ret;

    if (val) ret = am_hal_gpio_state_write(pad,AM_HAL_GPIO_OUTPUT_SET);
    else ret = am_hal_gpio_state_write(pad,AM_HAL_GPIO_OUTPUT_CLEAR);

    if (ret == AM_HAL_STATUS_SUCCESS) return(true);

    return(false);
}

/* read input value from pad
 * parameter pad : pad number on Apollo3
 * Return : true = high, false is low
 */
bool APread(uint32_t pad) {

    uint32_t ReadState = 0;

    if(am_hal_gpio_state_read(pad, AM_HAL_GPIO_INPUT_READ, &ReadState) == AM_HAL_STATUS_SUCCESS){
        if (ReadState) return(true);
    }

    return(false);
}

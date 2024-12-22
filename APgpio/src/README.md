# GPIO input /output settings

## Background

The Apollo3 chip has different GPIO setting to adjust the input and output pad. However due to the “Arduino-like” compatibility, not all of these options can be addressed with the default / standard code. This code is working directly with the HAL (Hardware Abstraction Layer) to perform different settings. The code has been tested on both the V1.2.1 and V2.0.6 boards library from Sparkfun.

## Disclaimer

During testing the code I accidentally damaged one of the pad (41) on my ATP board. Oh well..XXX happens. BUT I want to warn you to be careful and be fully aware that using any open source code, like this one, you are on your own: No Support, No warranty, NO help with damage. In legal terms :

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## Pin or Pad

A “pin” is the name of the place where you connect your wire on a board. A pad is the connection point of the Apollo3 processor to the board. While “pin” does not have to be the same as a “pad” on Apollo3 board it nearly always is. Often there is confusion whether to use a pinName (like D3) or a pin / pad-number (like 3). When addressing the HAL it is always the pad-number, the layer(s) above will translate a pinName to a pad-number.

Recently MBED started to define a standard approach to the definition of “pins”. As V2.x of Sparkfun boards library is using MBED-OS, one should expect some impact of that in the future. For more information see : [https://os.mbed.com/blog/entry/Improved-Pin-names-for-Mbed-boards/]

## Datasheet specifications

A standard Apollo3 chip has 50 Input / Output pads. Depending on the board (Edge, ATP etc) not all the pads are available as a pin for the user. There is also a compact Apollo3 (CSP) which has less pads available. Each pad can be set (value 0 to 7) to perform a different function. Functions like UART0RX or UART1TX, ADC, SDA, SCL,PDM, RTS, CTS etc etc. The complete overview can be found in the Apollo3 datasheet Table 559: Apollo3 Blue MCU Pad Function Mapping. All of them can perform the function of GPIO, by setting a FunctionSelect value of 3.

Many of the pads have a special hardware / attributes to perform a function.

All pads, except for pad 20, have a weak pull-up on the pad when pull-up has been set. For pad 20 a weak pull-down on the pad when set.

The Pads 0, 1, 5, 6, 8, 9, 25, 27, 39, 40, 42, 43, 48 and 49 include additional optional pull-up resistors for use in I2C mode to eliminate the need for external resistors. The resistor values that are available are 1.5K, 6K, 12K, 24K.

Nominal drive strengths of 2, 4, 8 or 12 mA can be selected for each pad. Next to that pads 3 and 36 have selectable high side power switch transistors to provide ~1 Ω switches to VDDH (3V3). Pads 37 and 41 have a selectable low side power switch transistors to provide ~1 Ω switches to VSS.(GND) The maximum allowable current is 100mA.

## Standard pinMode command.

With the command pinMode: pinMode(pin, mode), the following are the standard Arduino-like modes to set a GPIO:

INPUT: this will set a pin as input in a high-impedance state. Input pins make small demands on the circuit that they are sampling.

INPUT_PULLUP: This sets as INPUT on the Apollo3 and a weak-pullup is set by default of around 63kohm. This can cause a high impact on the circuit. Comparing to an Arduino which has between 20K and 150Kohm (depending on the board/processor used). It will check, and generate an error, if you try to set it on pin 20 (which only has pull-down). Often however these errors are neglected and not passed back to the sketch however.

INPUT_PULLDOWN: This sets as INPUT on the Apollo3 but a weak-pulldown is set by default of around 63kohm. This can cause a high impact on the circuit. It will check, and generate an error, if you try to set it on another pin than 20 (which is the only with pull-down). Often however these errors are neglected and not passed back to the sketch however.

OUTPUT: This will set the pin as output with a strength of 12mA. It will use PUSH-PULL, which will allow the ability to pull the output to both source (3v3)  and sink (GND) current. As a result the line will not be “floating”.

More info on Arduino pinMode() [https://www.arduino.cc/en/Tutorial/Foundations/DigitalPins]

## Using Apollo3 extra options

Some people want to be able to use the extra capabilities the Apollo3 chip has, but this can not be achieved with the standard pinMode(). The Hardware Abstraction Layer (HAL) has the functions to enable those capabilities on the Apollo3 chip. I have developed users level code to connect directly to the HAL. **As a result however, a number of checks that are normally performed are by-passed and the developer needs to know what he she is doing. Also your code is now only valid for an Apollo3 board running Sparkfun boards library.**

## Pad Configuration variables

As part of the structure am_hal_gpio_pincfg_t the following are the GPIO configuration variables.

### uFuncSel  Function select (FUNCSEL)
Each pad can be set (value 0 to 7) to perform a different function. For GPIO this is set to 3.

### ePowerSw  power switch source (VCC) or sink (VSS)
This can enable the power switch setting. The following options are allowed:

    AM_HAL_GPIO_PIN_POWERSW_NONE : NO power switch setting
    AM_HAL_GPIO_PIN_POWERSW_VDD  : enable power switch, only valid on pad  37 and 41
    AM_HAL_GPIO_PIN_POWERSW_VSS  : enable power switch, only valid on pad  3 and 36

### ePullup  will enable a pullup resistor
This will enable a pull-up resistor on a pad . The following options are allowed:

    AM_HAL_GPIO_PIN_PULLUP_NONE  :  No Pull-up
    AM_HAL_GPIO_PIN_PULLUP_WEAK  :  pullup is weak (~63K)
    AM_HAL_GPIO_PIN_PULLUP_1_5K  :  Pull-up is 1K5
    AM_HAL_GPIO_PIN_PULLUP_6K    :  Pull-up is 6K
    AM_HAL_GPIO_PIN_PULLUP_12K,  :  Pull-up is 12K
    AM_HAL_GPIO_PIN_PULLUP_24K,  :  Pull-up is 24K
    AM_HAL_GPIO_PIN_PULLDOWN     :  Pulldown  (~63K)

Weak pullup can be set on all pads except pad 20. Pulldown is only allowed on pad 20.
Pullup 1.5K – 24K can only be set on pads 0, 1, 5, 6, 8, 9, 25, 27, 39, 40, 42, 43, 48 and 49.

### eDriveStrength  Pad strength designator
This will set the drive strength in mA. The following options are allowed :

    AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA  : 2mA
    AM_HAL_GPIO_PIN_DRIVESTRENGTH_4MA  : 4mA
    AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA  : 8mA
    AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA : 12mA


### eGPOutcfg OUTPUT configuration
This will set the pad in an output state. The following options are allowed :

    AM_HAL_GPIO_PIN_OUTCFG_DISABLE	   : do NOT set as output
    AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL
    AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN
    AM_HAL_GPIO_PIN_OUTCFG_TRISTATE

source : [https://open4tech.com/open-drain-output-vs-push-pull-output/]

Push-pull is the most common output configuration. Just as its name suggests, output is capable of driving two output levels. One to ground (pull/sink current from the load) and the other is push to supply voltage (push/source current to the load).

In open drain configuration, the logic behind the pin can drive it only to ground (logic 0). The other possible state is high impedance (Hi-Z). If its drain terminal is open (the device is off) the pin is left floating to Hi-Z state.

Source :[https://electronics.stackexchange.com/questions/466788/what-is-the-advantage-of-a-tri-state-output]
Tri-state is essentially 3 different states.. Instead of calling it ON and OFF, think of it as IN and OUT instead. The third state is not floating, it is in a high impedance state, which is essentially disconnecting it from the circuit. This means your 3 states are:
    • Input
    • Output
    • High Impedance (disconnected)
This is very useful in circuits that have a common bus for multiple components. With tri-state logic, components can either read from the bus, write to the bus, or be essentially disconnected so that it does not affect the bus, and will not be affected by the state of the bus either.

### eGPInput GPIO Input
This will set the pad in an input state. The following options are allowed :

    AM_HAL_GPIO_PIN_INPUT_AUTO    : available but NOT used by HAL
    AM_HAL_GPIO_PIN_INPUT_NONE    : Do not set pad as input
    AM_HAL_GPIO_PIN_INPUT_ENABLE  : enable pad as input

### eIntDir  Interrupt direction
This will set the pad interrupt trigger. The following options are allowed :

    AM_HAL_GPIO_PIN_INTDIR_LO2HI  : trigger on low to high
    AM_HAL_GPIO_PIN_INTDIR_HI2LO  : trigger on high to low
    AM_HAL_GPIO_PIN_INTDIR_NONE   : do not trigger
    AM_HAL_GPIO_PIN_INTDIR_BOTH   : trigger on level change
    0x0                           : enable reading

To use this from a sketch would need to define the interrupt routine that is called. It is much easier to us attachInterrupt(). Do not use  AM_HAL_GPIO_PIN_INTDIR_NONE as that will result in always read as zero.

### eGPRdZero GPIO read as zero
This will set the pad for readback will always be zero. The following options are allowed :
    AM_HAL_GPIO_PIN_RDZERO_READPIN : read the real pad value
    AM_HAL_GPIO_PIN_RDZERO_ZERO    : always return zero

Not really see much of a value in this, but in case you want to use this make sure that eIntDir is set as  AM_HAL_GPIO_PIN_INTDIR_NONE.

## Example settings
```
### Output pad
.uFuncSel = 3,                                          // set pin as GPIO
.ePowerSw = AM_HAL_GPIO_PIN_POWERSW_NONE,               // no power switch
.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE,                 // no pullup
.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,   // 12mA max output
.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,           // A push-pull GPIO
.eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
.eIntDir = AM_HAL_GPIO_PIN_INTDIR_NONE,                 // NO interrupts
.eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read.. read as zeros

### Input pad with pull-up
.uFuncSel = 3,                                          // set pin as GPIO
.ePowerSw = AM_HAL_GPIO_PIN_POWERSW_NONE,               // no power switch
.ePullup = AM_HAL_GPIO_PIN_PULLUP_12K,                  // 12K pullup
.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,    // ignored anyway
.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_DISABLE,            // No output
.eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
.eIntDir = 0x0,                                         // NO interrupts
.eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read the pad

only for pads 0, 1, 5, 6, 8, 9, 25, 27, 39, 40, 42, 43, 48 and 49

### Power switch VDD
.uFuncSel = 3,                                          // set pin as GPIO
.ePowerSw = AM_HAL_GPIO_PIN_POWERSW_VDD,                // no power switch
.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE,                 // no pullup
.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,   // 12mA max output
.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,           // A push-pull GPIO
.eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
.eIntDir = AM_HAL_GPIO_PIN_INTDIR_NONE,                 // NO interrupts
.eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read.. read as zeros

Note : only valid on pad 37 and 41.

### Power switch VSS
.uFuncSel = 3,                                          // set pin as GPIO
.ePowerSw = AM_HAL_GPIO_PIN_POWERSW_VSS,                // no power switch
.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE,                 // no pullup
.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,   // 12mA max output
.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,           // A push-pull GPIO
.eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
.eIntDir = AM_HAL_GPIO_PIN_INTDIR_NONE,                 // NO interrupts
.eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read.. read as zeros

Note : only valid on pad 3 and 36
```

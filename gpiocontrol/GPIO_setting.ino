/* 
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
 *  Version 1.0./ April 2021 / Paulvha
 */

// valid for all pads
const am_hal_gpio_pincfg_t PadDef12 =
{
  .uFuncSel = 3,                                          // set pin as GPIO
  .ePowerSw = AM_HAL_GPIO_PIN_POWERSW_NONE,               // no power switch
  .ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE,                 // no pullup
  .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,   // 12mA max output
  .eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,           // A push-pull GPIO has the ability to both source and sink current
  .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
  .eIntDir = AM_HAL_GPIO_PIN_INTDIR_NONE,                 // NO interrupts
  .eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read.. read as zeros
};

// Input valid for all pads except pad 20
const am_hal_gpio_pincfg_t PadDefInput =
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

// Input only valid for pads 0, 1, 5, 6, 8, 9, 25, 27, 39, 40, 42, 43, 48 and 49. 
const am_hal_gpio_pincfg_t PadDefInputPullup =
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
const am_hal_gpio_pincfg_t PadDefVDD =
{
  .uFuncSel = 3,                                          // set pin as GPIO
  .ePowerSw = AM_HAL_GPIO_PIN_POWERSW_VDD,                // power switch to 3v3
  .ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE,                 // no pullup
  .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,    // weak
  .eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,           // A push-pull GPIO has the ability to both source and sink current
  .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
  .eIntDir = AM_HAL_GPIO_PIN_INTDIR_NONE,                 // NO interrupts
  .eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read.. read as zeros
};

// Output only valid for Pad 37 and 41
const am_hal_gpio_pincfg_t PadDefVSS =
{
  .uFuncSel = 3,                                          // set pin as GPIO
  .ePowerSw = AM_HAL_GPIO_PIN_POWERSW_VSS,                // power switch GND
  .ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE,                 // no pullup
  .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,    // weak
  .eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,           // A push-pull GPIO has the ability to both source and sink current
  .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,               // enable for input
  .eIntDir = AM_HAL_GPIO_PIN_INTDIR_NONE,                 // NO interrupts
  .eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN             // when read.. read as zeros
};

uint32_t Pad = 17;         // use pad number output
uint32_t PadVDD = 3;       // use pad number output power supply to VDD
uint32_t PadVSS = 41;      // use pad number output power supply to VSS
uint32_t PadInp = 45;      // use pad number for input
uint32_t PadInpPullup = 1; // use pad number for input with Pullup

void setup() {
  Serial.begin(115200);
  
  do {
    delay(500);
  }while(!Serial);
 
  Serial.println("Demo code for setting GPIO");

  //*********** set output
  if (am_hal_gpio_pinconfig(Pad, PadDef12) == AM_HAL_STATUS_SUCCESS){
    Serial.printf("pad %d has been set for 12mA\n", Pad);
    am_hal_gpio_state_write(Pad,AM_HAL_GPIO_OUTPUT_SET);
  }
  else
    Serial.printf("Error during setting pad %d as been set for 12mA\n", Pad);
    
  //*********** set output VDD
  if (am_hal_gpio_pinconfig(PadVDD, PadDefVDD) == AM_HAL_STATUS_SUCCESS) {
    Serial.printf("pad %d has been set for power-switch to VDD\n", PadVDD);
    am_hal_gpio_state_write(PadVDD,AM_HAL_GPIO_OUTPUT_SET);
  }
  else
    Serial.printf("Error during setting pad %d as been set for power-switch to VDD\n", PadVDD);

  //*********** set output VSS
  if (am_hal_gpio_pinconfig(PadVSS, PadDefVSS) == AM_HAL_STATUS_SUCCESS){
    Serial.printf("pad %d has been set for power-switch to GND\n", PadVSS);
    
    // set for CLEAR as it needs to connect to GND !!
    am_hal_gpio_state_write(PadVSS,AM_HAL_GPIO_OUTPUT_CLEAR);
  }
  else
    Serial.printf("Error during setting pad %d as been set for power-switch to GND\n", PadVSS);

  //*********** set input
  if (am_hal_gpio_pinconfig(PadInp, PadDefInput) == AM_HAL_STATUS_SUCCESS){
    Serial.printf("pad %d as been set for input\n", PadInp);
  }
  else
    Serial.printf("Error during setting pad %d as input\n", PadInp);

  //*********** set input with Pullup
  if (am_hal_gpio_pinconfig(PadInpPullup, PadDefInputPullup) == AM_HAL_STATUS_SUCCESS) {
    Serial.printf("pad %d as been set as input with pullup\n", PadInpPullup);
  }
  else
    Serial.printf("Error during setting pad %d as input with Pullup\n", PadInpPullup);
}

void loop() {
  
  // is floating... might read high or low without solid connection
  Serial.printf("\npad %d has input level %s. With digitalRead %s\n",PadInp,ReadPad(PadInp, 1)? "high":"low", digitalRead(PadInp)? "high":"low");

  // due to pull-up should read HIGH, unless tied to GND with a wire
  // do NOT use digitalRead() on 2.0.6 as it will remove pullup enablement
  Serial.printf("\npad %d (with pullup) has input level %s\n",PadInpPullup,ReadPad(PadInpPullup, 1)? "high":"low");

  delay(5000);
}

/*
 * pad: pad to read
 * type : 
 * 0 = read enabled state
 * 1 = read input state
 * 2 = read output state
 *
 */
bool ReadPad(uint32_t inpad, int type)
{
   uint32_t ReadState = 0;
   am_hal_gpio_read_type_e t;
   
   switch(type){
    case 0 : 
      t = AM_HAL_GPIO_ENABLE_READ;
      break;
    case 1 : 
      t = AM_HAL_GPIO_INPUT_READ;
      break;
    default : 
      t = AM_HAL_GPIO_OUTPUT_READ;
      break;     
   }
   
   if(am_hal_gpio_state_read(inpad,t, &ReadState) == AM_HAL_STATUS_SUCCESS){
    if (ReadState) return(true);
   }
   
   return(false);
}

/*
  Ability to use Artemis/ Apollo3 extended GPIO functions

  By: Paul van Haastrecht
  Date: 1 May 2021
  This example code is in the public domain

  Feel like supporting open source hardware?
  Buy a board from SparkFun! https://www.sparkfun.com/search/results?term=artemis

  This example shows
  How to set a pin with power switch. The maximum current to draw is 100mA

  Allowed modes:
  M_VDD,   M_VSS,  M_INPUT,  M_OUTPUT,  M_PULLUP,  M_PULLDOWN

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include "APgpio.h"

// define the pads to use
uint32_t  padVDD = 3;     // pads 3 or 36 are alllowed
uint32_t  padVSS = 41;    // Pads 37 or 41 are alllowed

void setup() {

  Serial.begin(115200);

  do {
    delay(500);
  }while(!Serial);

  Serial.println("Example3: setting Power Switches");

  // set pin as power switch to 3V3
  if (APmode(padVDD, M_VDD)) Serial.printf("Pad %d has been set with PowerSwitch to 3V3\n", padVDD);
  else {
    Serial.printf("Pad %d could not be set with PowerSwitch to 3V3\n", padVDD);
    while(1);
  }

  // set pin as power switch to GND
  if (APmode(padVSS, M_VSS)) Serial.printf("Pad %d has been set with PowerSwitch to GND\n", padVSS);
  else {
    Serial.printf("Pad %d could not be set with PowerSwitch to GND\n", padVSS);
    while(1);
  }
}

void loop() {

  if (APset(padVDD, HIGH)) Serial.printf("\nPad %d (VDD) has been set HIGH\n", padVDD);
  else Serial.printf("Pad %d (VDD) could not be set HIGH\n", padVDD);
  Serial.printf("pad %d (VDD) level reads as %s.\n",padVDD,APread(padVDD)? "high":"low");

  if (APset(padVSS, HIGH)) Serial.printf("Pad %d (VSS) has been set HIGH\n", padVSS);
  else Serial.printf("Pad %d (VSS) could not be set HIGH\n", padVSS);
  Serial.printf("pad %d (VSS) level reads as %s.\n",padVSS,APread(padVSS)? "high":"low");

  delay(2000);

  if (APset(padVDD, LOW)) Serial.printf("Pad %d (VDD) has been set LOW\n", padVDD);
  else Serial.printf("Pad %d (VDD) could not be set LOW\n", padVDD);
  Serial.printf("pad %d (VDD) level reads as %s.\n",padVDD,APread(padVDD)? "high":"low");

  if (APset(padVSS, LOW)) Serial.printf("Pad %d (VSS) has been set LOW\n", padVSS);
  else Serial.printf("Pad %d (VSS) could not be set LOW\n", padVSS);
  Serial.printf("pad %d (VSS) level reads as %s.\n",padVSS,APread(padVDD)? "high":"low");

  delay(2000);
}

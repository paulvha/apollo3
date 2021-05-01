/*
  Ability to use Artemis/ Apollo3 extended GPIO functions

  By: Paul van Haastrecht
  Date: 1 May 2021
  This example code is in the public domain

  Feel like supporting open source hardware?
  Buy a board from SparkFun! https://www.sparkfun.com/search/results?term=artemis

  This example shows :
  how to set as a pin as input
  set another as input with pullup.
  Set pin 20 with pulldown

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
uint32_t  padIN = 45;
uint32_t  padPullup = 1;
uint32_t  padPulldown = 20;

void setup() {

  Serial.begin(115200);

  do {
    delay(500);
  }while(!Serial);

  Serial.println("Example1: setting three different input GPIO");

  // set pin as input without a pullup
  if (APmode(padIN,M_INPUT))
    Serial.printf("Pad %d has been set as input\n", padIN);
  else {
    Serial.printf("Pad %d could not be set as input\n", padIN);
    while(1);
  }

  // set pin as input with pull-up. allowed pullup resistors : R_1_5, R_6, R_12 or R_24 (Kohm)
  if (APmode(padPullup,M_PULLUP, R_12))
    Serial.printf("Pad %d has been set as pull-up\n", padPullup);
  else {
    Serial.printf("Pad %d could not be set as pull-up\n", padPullup);
    while(1);
  }

  // set pin as input without a down (pad 20 only)
  // On ATP board pad 20 is pad of the JTAG connector
  if (APmode(padPulldown, M_PULLDOWN))
    Serial.printf("Pad %d has been set as input with pull-down\n", padPulldown);
  else {
    Serial.printf("Pad %d could not be set as input with pull-down\n", padPulldown);
    while(1);
  }
}

void loop() {

  // is floating... might read high or low without solid connection
  Serial.printf("\npad %d has input level %s.\n",padIN,APread(padIN)? "high":"low");

  // due to pull-up should read HIGH, unless tied to GND with a wire
  Serial.printf("pad %d (with pullup) has input level %s\n",padPullup,APread(padPullup)? "high":"low");

  // due to pull-down should read LOW, unless tied to 3V3 with a wire
  Serial.printf("pad %d (with pulldown) has input level %s\n",padPulldown,APread(padPulldown)? "high":"low");

  delay(5000);
}

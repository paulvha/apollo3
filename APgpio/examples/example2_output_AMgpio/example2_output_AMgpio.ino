/*
  Ability to use Artemis/ Apollo3 extended GPIO functions

  By: Paul van Haastrecht
  Date: 1 May 2021
  This example code is in the public domain

  Feel like supporting open source hardware?
  Buy a board from SparkFun! https://www.sparkfun.com/search/results?term=artemis

  This example shows how to set a pin as output and set a different drive strength

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

// define the pad to use
uint32_t  padOut = 45;

void setup() {

  Serial.begin(115200);

  do {
    delay(500);
  }while(!Serial);

  Serial.println("Example2: setting output GPIO");

  // set pin as output. Arguments are D_2, D_4, D_8, D_12 (mA)
  // if argument is ommitted D_12 will be set

  if (APmode(padOut,M_OUTPUT, D_8))
    Serial.printf("Pad %d has been set as output\n", padOut);
  else
    Serial.printf("Pad %d could not be set as output\n", padOut);
}

void loop() {

  if (APset(padOut, HIGH)) Serial.printf("\nPad %d has been set HIGH\n", padOut);
  else Serial.printf("Pad %d could not be set HIGH\n", padOut);

  Serial.printf("pad %d level reads as %s.\n",padOut,APread(padOut)? "high":"low");

  delay(2000);

  if (APset(padOut, LOW)) Serial.printf("Pad %d has been set LOW\n", padOut);
  else Serial.printf("Pad %d could not be set LOW\n", padOut);

  Serial.printf("pad %d level reads as %s.\n",padOut,APread(padOut)? "high":"low");

  delay(2000);
}

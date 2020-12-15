/* 

Temporarly change the Bluetooth Device address on Apollo3

Version 1.0 / November 2020 / paulvha
- initial version

********************************************************************************************************
THE PROBLEM

Each Apollo3 'should' have a unique Bluetooth address, which is stored in the OTP (One-Time-Programmable) 
memory of the Bluetooth chip which is within the Apollo3 housing. 
I say SHOULD because it is not unique. Many have seem the same address: both my Edge boards have: 
56:77:88:23:AB:EF or others have 66:77:88:23:BB:EF. Not a problem if you only have one board on the 
Bluetooth network, but if you have multiple with the same device address it will not work. Personally I 
think this is a massive error in Apollo3 as devices addresses are supposed to be unique

REMEMBER

This sketch allows you to change the Bluetooth device address in RAM on the Bluetooth chip,
but it is restored to what is set in OTP after each reset. As such the code MUST run on each board, 
with a unique address set for each board in the sketch during compile. !!!!!!

!! BEFORE YOU START!! 

I have tested this on Sparkfun Apollo3 library version 2.0.2 and ArduinoBLE version 1.1.3. 
Originally I had made changes to ArduinoBLE to include the WriteNewBdAddr() call, 
but if there is a new version of that library those changes will be lost.  
Hence a different approach.

Before you can compile it requires a SMALL CHANGE  in ArduinoBLE/src/uility/HCI.h. 

Around line 80 you will find :

private : 
int sendCommand(uint16_t opcode, uint8_t plen = 0, void* parameters = NULL); 

Now move the sendCommand() to the line ABOVE private, into the public area so we can call 
this function from the sketch:

int sendCommand(uint16_t opcode, uint8_t plen = 0, void* parameters = NULL); 
private:

(OPTIONAL) SELECT THE CORRECT BLUETOOTH CHIP

It seems (BUT I have NO confirmation about this) there are potential different Bluetooth chips in the 
different Apollo3 : EM9304 or NATIONZ. They use different vendor specific codes for setting the Bluetooth 
address. The NATIONZ code worked in my 3 boards. (2 x EDGE, 1 x ATP), so I have left NATIONZ as default, 
but if that fails on your board try to use the EM9304 code in WriteNewBdAddr()

HOW TO USE

Define your new Bluetooth device address in NEW_APOLLO_BDADR in REVERSE order : 
{0x66, 0x55, 0x44, 0x33, 0x22, 0x11} will become device address 11:22:33:44:55:66

Compile and run the sketch. It will show the OTP stored Bluetooth address,  
perform a change and then display the updated address.

******************************************************************************************

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

  No support, be carefull you are on your own. I only hope it helps !!
*/

#include <ArduinoBLE.h>
#include "utility/HCI.h"    // needed for sendcommand

// set the name
#define BLE_PERIPHERAL_NAME "Artemis BLE"

// set new Device Address
#define NEW_APOLLO_BDADR   {0x66, 0x55, 0x44, 0x33, 0x22, 0x11}
static uint8_t BLEMacAddress[6] = NEW_APOLLO_BDADR;

//*****************************************************************************

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println(F("Starting Bluetooth"));
  
  BLE.debug(Serial);   // optional debug HCI messages
  
  // begin initialization
  if (!BLE.begin()) {
    Serial.println(F("starting BLE failed!"));
    while (1);
  }
  
  // set the local name peripheral advertises
  BLE.setLocalName(BLE_PERIPHERAL_NAME);

  Serial.println(("\r\nBluetooth device address BEFORE..."));
  String address = BLE.address();
  Serial.print("local address ");  Serial.println(address);
  Serial.print("local name ");  Serial.println(BLE_PERIPHERAL_NAME);
  
  if (! WriteNewBdAddr()){
    Serial.println(F("Failed to set new Device Address!"));
    Serial.println(F("Maybe select another chip in WriteNewBdAddr()"));
    while (1);
  }

  Serial.println(("\r\nBluetooth device address AFTER..."));
  String address1 = BLE.address();
  Serial.print("local address ");  Serial.println(address1);
  Serial.print("local name ");  Serial.println(BLE_PERIPHERAL_NAME);
}

void loop() {
  // put your main code here, to run repeatedly:

}

// write new address with vendor specific command
bool WriteNewBdAddr()
{
  // NATIONZ
  // if sendcommand can not be found by the compiler, you have to read the 
  // top part of the sketch first for a small change
  int result = HCI.sendCommand(0xFC32, 6, BLEMacAddress);

  // EM9304
  //int result = HCI.sendCommand(0xFC02, 6, BLEMacAddress);
  
  if (result == 0) return true;

  return false;
}

/*
 * Read the registers from MLX90614 in SMBus mode
 * 
 * Version 1.0 / November 2021 / paulvha
 * 
 * // defaults output
 * MLX90614_REGISTER_TA      (0x6)   0x3974
 * MLX90614_REGISTER_TOBJ1   (0x7)   0x39B5
 * MLX90614_REGISTER_TOBJ2   (0x8)   0x0
 * MLX90614_REGISTER_TOMAX   (0x20)  0x9993
 * MLX90614_REGISTER_TOMIN   (0x21)  0x62E3
 * MLX90614_REGISTER_PWMCTRL (0x22)  0x201
 * MLX90614_REGISTER_TARANGE (0x23)  0xF71C
 * MLX90614_REGISTER_KE      (0x24)  0xFFFF
 * MLX90614_REGISTER_CONFIG  (0x25)  0x9FB4
 * MLX90614_REGISTER_ADDRESS (0x2E)  0xBE5A
 * 
 *  ================================ Disclaimer ======================================
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  ===================================================================================
 */


#include <Wire.h> // I2C library, required for MLX90614
#include <SparkFunMLX90614.h>//Click here to get the library: http://librarymanager/All#Qwiic_IR_Thermometer by SparkFun

IRTherm therm; // Create an IRTherm object to interact with throughout

void setup() 
{
  Serial.begin(115200); 
  Wire.begin(); //Join I2C bus

  Serial.println("Read the MLX90614 registers.\r");
  Serial.println("This sketch CAN NOT BE USED ON ARTEMIS / APOLLO3.\r");
  
  if (therm.begin() == false){ // Initialize the MLX90614
    Serial.println("MLX90614 thermometer did not acknowledge! Freezing!");
    while(1);
  }
  Serial.println("MLX90614 IR thermometer acknowledged.");

  Serial.print("MLX90614_REGISTER_TA ");
  ReadReg(MLX90614_REGISTER_TA);
  Serial.print("MLX90614_REGISTER_TOBJ1 ");
  ReadReg(MLX90614_REGISTER_TOBJ1);
  Serial.print("MLX90614_REGISTER_TOBJ2 ");
  ReadReg(MLX90614_REGISTER_TOBJ2);  
  Serial.print("MLX90614_REGISTER_TOMAX ");
  ReadReg(MLX90614_REGISTER_TOMAX);
  Serial.print("MLX90614_REGISTER_TOMIN ");
  ReadReg(MLX90614_REGISTER_TOMIN);
  Serial.print("MLX90614_REGISTER_PWMCTRL ");
  ReadReg(MLX90614_REGISTER_PWMCTRL); 
  Serial.print("MLX90614_REGISTER_TARANGE ");
  ReadReg(MLX90614_REGISTER_TARANGE);
  Serial.print("MLX90614_REGISTER_KE ");
  ReadReg(MLX90614_REGISTER_KE);
  Serial.print("MLX90614_REGISTER_CONFIG ");
  ReadReg(MLX90614_REGISTER_CONFIG); 
  Serial.print("MLX90614_REGISTER_ADDRESS ");
  ReadReg(MLX90614_REGISTER_ADDRESS);


  // in case you want to update a register use the following
  // command from the library
  /*
  uint16_t val = 0x9FB4;

  if( !therm.writeEEPROM(MLX90614_REGISTER_CONFIG, val))
  {
   Serial.println(F("Error during writing MLX90614_REGISTER_CONFIG. Freeze\r"));
   while(1); 
  }
  */
}


void loop() {
  // put your main code here, to run repeatedly:

}

void ReadReg(byte reg)
{
  int16_t toVal;
  // Read from the TOMax EEPROM address, store value in toMax
  if (therm.I2CReadWord(reg, &toVal))
  {
    Serial.print("\t(0x");
    Serial.print(reg, HEX);
    Serial.print(")\t0x");
    Serial.println(toVal & 0xffff, HEX);
    return;
  }
  Serial.println(" ERROR DURING READING. FREEZE\r");
  while(1);
}

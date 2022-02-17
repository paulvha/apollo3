/**
 * Reading and writing the MicroSD on different Artemis platforms 
 * 
 * MicroMod MainBoard (DEV-18576) with MM Artemins Processor (WRL-16781)
 * MicroMod Data logger carrier board (DEV-16829) with MM Artemins Processor (WRL-16781)
 * Artemis OpenLog (DEV-16832)
 * Artemis ATP (DEV-15442) with external SparkFun Level Shifting microSD (DEV-13743)
 * 
 * Parts of the code has been taken from the OpenLog sketch/program created by Sparkfun
 * 
 * 
 * Versioning :
 *  
 *  1.2 add support for Micromod datalogger & MicroMod main board / February 2022
 *  
 *  1.1 added support for ATP               / December 2021
 *  
 *  1.0 initial version for OpenLog         / December 2021
 * 
 * 
 * connections
 * 
 * External   Artemis
 * MicroSD    ATP
 * VCC        3V3 
 * D2/CS      23  (defined by PIN_MICROSD_CHIP_SELECT)
 * D1         MOSI / COPI
 * SCK        SCK
 * D0         MISO / CIPO
 * CD         27  (defined by PIN_MICROSD_CD_SELECT)
 * GND        GND
 * 
 *            Artemis
 * MicroSD    MICROMOD_MMDLCB  (FYI only fixed)
 * VCC        33  (define by PIN_MICROSD_POWER)
 * D2/CS      41 (on-board) or 16 (on HEADER_CS) (defined by PIN_MICROSD_CHIP_SELECT)
 * D1         MOSI / COPI
 * SCK        SCK
 * D0         MISO / CIPO
 * CD         Not connected (on-board) / not used (on HEADER_CS)
 * GND        GND
 * 
 *            Artemis
 * MicroSD    MICROMOD_MAIN baord  (FYI only fixed)
 * D2/CS      28
 * D1         MOSI / COPI
 * SCK        SCK
 * D0         MISO / CIPO
 * CD         Not connected
 * GND        GND 
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

#include <SPI.h>
#include <SdFat.h> //SdFat v2.0.7 by Bill Greiman: http://librarymanager/All#SdFat_exFAT

/*********************************************************************************
 Define whether this is used on Artemis OpenLog, MICROMOD_MMDLCB, MAINB or ATP
 On OpenLog & Micromod MMDLCB we need to switch on the MicroSD power
 On ATP the external microSD has CD detect function
 on Micromod Main board, no CD, NO power switch */
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// MAKE SURE TO SELECT THE RIGHT BOARD HERE AND IN ARDUINO IDE -> TOOLS -> BOARD
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define ARTEMIS_OPENLOG 1                   // select Artemis Openlog board
//#define MICROMOD_MMDLCB 1                 // Micromod data logger carrier board
//#define MICROMOD_MAINB  1                 // Micromod mainboard
//#define ATP                               // ATP with external microSD

#ifdef MICROMOD_MMDLCB
const byte PIN_MICROSD_POWER = 33;          // LDO power
const int sdPowerDownDelay = 100;           // Delay for this many ms before turning off the SD card power
const byte PIN_MICROSD_CHIP_SELECT = 41;    // CS (chip select IOM)
#endif //MICROMOD_MMDLCB

#ifdef MICROMOD_MAINB
const byte PIN_MICROSD_CHIP_SELECT = 28;    // CS (chip select IOM)
#endif //MICROMOD_MAINB

#ifdef ARTEMIS_OPENLOG
const byte PIN_MICROSD_POWER = 15;          // power the FET
const int sdPowerDownDelay = 100; //Delay for this many ms before turning off the SD card power
const byte PIN_MICROSD_CHIP_SELECT = 23;    // CS (chip select)
#endif //ARTEMIS_OPENLOG

#ifdef ATP
const byte PIN_MICROSD_CD_SELECT = 27;      // Connect CD here (card detect)
const byte PIN_MICROSD_CHIP_SELECT = 23;    // Connect CS here (chip select)
#endif // ATP

bool online_microSD = false;

#define SD_FAT_TYPE 3 // SD_FAT_TYPE = 0 for SdFat/File, 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_CONFIG SdSpiConfig(PIN_MICROSD_CHIP_SELECT, SHARED_SPI, SD_SCK_MHZ(24)) // 24MHz

#if SD_FAT_TYPE == 1
SdFat32 sd;
File32 sd_root; //current dir
File32 sd_file; //current direcotory entry
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile sd_root; //current dir
ExFile sd_file; //current direcotory entry
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile sd_root; //current dir
FsFile sd_file; //current direcotory entryto
#else // SD_FAT_TYPE == 0
SdFat sd;
File sd_root; //current dir
File sd_file; //current direcotory entry
#endif  // SD_FAT_TYPE

// needed to display directory
#define MAXFILENAME 250      // maximum length file or directory name     
#define MAX_DIR_ENTRY 30     // maximum entries in current directory

struct entry{
    char name[MAXFILENAME];  // name of directory entry
    bool isDir;              // true if directory
};

struct entry DirContent[MAX_DIR_ENTRY];
char fname[MAXFILENAME];
uint16_t pdate, ptime;      // get date and time

// needed to go directory levels down and up
char RootDir[MAX_DIR_ENTRY * 5];
char TempDir[MAX_DIR_ENTRY * 5];

int sel, offset;

void setup() {

  Serial.begin(115200); 
  while (!Serial); 

  Serial.println(F("MicroSD menu example. Version 1.2\nPress Enter to continue."));
  GetFileName();
  
  beginSD();

  if(!online_microSD) {
    Serial.println("something went wrong during initialize SD. Freeze !");
    while(1);
  }

  // init list
  DirContent[0].name[0] = 0x0;

  // get first file list
  list_files(false,true);
}

void loop() {
  
  Serial.println(F("\n\n1  = list the files in current directory"));
  Serial.println(F("2  = Change directory"));
  Serial.println(F("3  = Create new directory"));
  Serial.println(F("4  = Create new file"));
  Serial.println(F("5  = Back to root directory"));
  Serial.println(F("6  = Remove file"));
  Serial.println(F("7  = Remove directory"));
  Serial.println(F("8  = One level up"));
  Serial.println(F("9  = Read file. Display in ASCII only"));
  Serial.println(F("10 = Read file. Display in HEX and ASCII"));
  Serial.println(F("15 = Close SD"));

  if (GetEntryNum() == -1) return;
  
  switch (sel) {
    case 1:
      list_files(true, false);
      break;
    case 2:
      change_dir();
      break;      
    case 3:
      create_file(true);
      break;   
    case 4:
      create_file(false);
      break;
    case 5:
      list_files(true, true);
      break;
    case 6:
      remove_file(false);
      break;
    case 7:
      remove_file(true);
      break;
    case 8:
      OneLevelUP();
      break;
    case 9:
      Read_file(false);
      break;
    case 10:
      Read_file(true);
      break;  
    case 15:
      digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
      
#if (defined ARTEMIS_OPENLOG) || (defined MICROMOD_MMDLCB)
      delay(sdPowerDownDelay);
      microSDPowerOff();
#endif

      Serial.println("Press reset to restart");
      while(1);      
      break;
    default:
      Serial.print("Invalid input 0x");
      Serial.println(sel, HEX);
  }
}

/* read an entry number 
 *   
 * return
 *  sel     result is also stored in global variable sel.
 *  -1      invalid input or quit
 */
int GetEntryNum()
{
  flush();
  
  while (!Serial.available());
  char c = '0';
  sel = 0;
   
  while (c != '\r' && c != '\n') {
    
    if (c >= '0' && c <= '9') {
      sel = sel * 10;
      sel = sel + (c - '0');
      delay(10);
      c = Serial.read();
      Serial.print(c);
      if (c == 'q' || c == 'Q') return(-1);
    }
    else {
      Serial.print("\nInvalid number 0x");
      Serial.println(c, HEX);
      return(-1);
    }
  }
  Serial.println();
  return(sel);
}

/*
 * read a filename and store in global variable fname
 * @ return number of characters in fname
 *
 */
int GetFileName()
{
  flush();
  
  // wait for input
  while(!Serial.available()) yield();
  char c = Serial.read();

  while(c != '\r' && c != '\n' && offset < MAXFILENAME -1) {
    fname[offset++] = c;
    Serial.print(c);
    while (! Serial.available()) yield();
    c = Serial.read();
  }
  
  Serial.println();
   
  fname[offset] = 0x0;
  
  return(offset);
}

/*
 * flush any pending input
 */
void flush()
{
   delay(100);
   
   while (Serial.available()) {
    Serial.read();
    delay(50);
   }
}

/**
 * list a directory entry
 * @param show : will display the directory content as well
 * @param root : start from the root directory.
 */
bool list_files(bool show, bool root)
{
  offset = 0;
   
  if (root) sprintf(RootDir,"%s","/");  
  
  if (!sd.chdir("/")){
    Serial.println(F("Error during setting start position\n"));
    return false;    
  }
   
  if (!sd_root.open(RootDir)) {
    Serial.println(F("Error during opening SD card\n"));
    return false;
  }

  if (!sd_root.isDir()) {
    Serial.print (F("Error: "));
    Serial.print (RootDir);
    Serial.println (F(" Not a directory\n"));
    sd_root.close();
    return false;
  }

  sd_root.rewind();
  
  if (show) {
    Serial.print(F("Content of "));
    Serial.println(RootDir);
  }
  
  while (sd_file.openNext(&sd_root, O_RDONLY))
  {
    // skip hidden entry (e.g. volume directory)
    if (sd_file.isHidden()) {
      sd_file.close();
      continue;
    }
     
    // get name and size
    if (sd_file.getName(fname, MAXFILENAME)) {

      sprintf(DirContent[offset].name,"%s",fname);      // add entry in array
      DirContent[offset].isDir = sd_file.isDir();
    
      if (show) {
        Serial.print(offset);
        if (sd_file.isDir()) Serial.print("\tdir\t");
        else Serial.print("\tfile\t");
    
        // add index, time and date
        if (sd_file.getModifyDateTime(&pdate, &ptime)) {
          
          // Allow YYYY-MM-DD hh:mm
          char buf[sizeof("YYYY-MM-DD hh:mm ") -1];
          char* str = buf + sizeof(buf);
          str = fsFmtTime(str, ptime);
          *--str = '\t';
          str = fsFmtDate(str, pdate);
          str[16] = 0x0;               // terminate on right place          
          Serial.print(str);
        }
        
        Serial.print("\t");
        Serial.print("size");
     //   Serial.print(sd_file.fileSize());
        Serial.print("\t");
        Serial.println(fname);
      }
    }
    
    sd_file.close();

    offset++;
    
    if (offset == MAX_DIR_ENTRY) {
      Serial.println("WARNING : can not add more entrys in array\n");
      offset--;
    }
  }
  
  DirContent[offset].name[0] = 0x0;
  sd_root.close();
  return(true);
}

/*
 * change to a next level directory
 */
bool change_dir()
{
  for(offset = 0; offset < MAX_DIR_ENTRY; offset++){
    
    if (! strlen(DirContent[offset].name)) break;
    
    Serial.print(offset);
    Serial.print("\t");
    
    if (DirContent[offset].isDir) Serial.println(DirContent[offset].name);
    else Serial.println("file. can not select");
  }

  Serial.println("Which directory NUMBER do you want to select ? q = quit ");
  
  if (GetEntryNum() == -1) return false;
  
  if (sel > offset -1) {
    Serial.println("Invalid selection.");
    return false;
  }
  
  if (! DirContent[sel].isDir) {
    Serial.println("Not a valid directory.");
    return false;
  }
  
  if (strlen(RootDir) == 1)
    sprintf(TempDir,"%s",DirContent[sel].name);
  else
    sprintf(TempDir,"%s/%s", RootDir, DirContent[sel].name);
    
  sprintf(RootDir, "%s", TempDir);
 
  return(list_files(true,false));
}

/*
 * create a new file in the current directory
 */

bool create_file(bool dir)
{
  offset = 0 ;

  if (dir)
    Serial.print(F("Enter the name of the directory to create. Only enter is abort. "));
  else
    Serial.print(F("Enter the name of the file to create. Only enter is abort. "));

  if (GetFileName() == 0) return true;

  if (! sd.chdir(RootDir)) {
    Serial.print(F("Error during opening base directory "));
    Serial.println(RootDir);
    return false;
  }
  
  // check it exists ??
  if (sd_file.open(fname, O_READ))
  {
    Serial.println(F("Already exists"));
    sd_file.close();
    return false;
  } 
  
  sd_file.close();

  if(dir) {
     if ( ! sd.mkdir(fname, false)) {
    Serial.println(F("Error during creating directory"));
    }
    else {
      Serial.println(F("Directory has been created"));
      sd_file.close();
    }
  }
  else  // file
  {
    if (! sd_file.open(fname, O_CREAT | O_APPEND | O_WRITE))
    {
      Serial.println(F("Failed to create file"));
    }
    else {
      Serial.println(F("File has been created"));
      sd_file.close();
    }
  }
  
  return(list_files(true,false));
}

/*
 * remove file or directory
 * @param dir : true for directory
 */
bool remove_file(bool dir){
  
  uint8_t offset;

  for(offset = 0; offset < MAX_DIR_ENTRY; offset++){
    
    if (! strlen(DirContent[offset].name)) break;
    
    Serial.print(offset);
    Serial.print("\t");
    
    if (dir) {
      if (DirContent[offset].isDir)  Serial.println(DirContent[offset].name);
      else Serial.println(F("file. can not select."));
    }
    else {
      if (! DirContent[offset].isDir) Serial.println(DirContent[offset].name);
      else Serial.println(F("Directory. can not select."));
    }
  }
  
  if (dir)
    Serial.println(F("Which directory NUMBER do you want to select ? q = quit "));
  else
    Serial.println(F("Which file NUMBER do you want to select ? q = quit "));
  
  if (GetEntryNum() == -1) return false;
    
  if (sel > offset -1) {
    Serial.println("Invalid selection.");
    return false;
  }

  if (dir) {
    if (! DirContent[sel].isDir) {
      Serial.print(DirContent[sel].name);
      Serial.println(F(" Not a valid directory."));
      return false;
    }
    
    sprintf(TempDir,"%s/%s",RootDir, DirContent[sel].name); 
    
    if (!sd_root.open(TempDir)) {
      Serial.println(F("Error during opening SD card"));
      return false;
    }

    sd_root.rewind();

    uint8_t count = 0;
    
    while (sd_file.openNext(&sd_root, O_RDONLY))
    {
      count++;
      sd_file.close();
    }
    
    sd_root.close();
    
    if(count > 0) {
      Serial.println (F("ERROR. can not remove "));
      Serial.print(DirContent[sel].name);
      Serial.print(F(" as it has "));
      Serial.print(offset);
      Serial.println(F(" entry(s)"));
      return false;
    }
    else {
      Serial.print(DirContent[sel].name);
      Serial.println(F(" empty directory"));
    }
  }
  else {
  
    if (DirContent[sel].isDir) {
      Serial.print(DirContent[sel].name);
      Serial.println(F(" Not a valid file."));
      return false;
    } 
  }

  // double check
  Serial.print(F("Are you sure you want to remove: "));
  Serial.println( DirContent[sel].name);
  Serial.println(F("( Y or N ). Must be a CAPITAL to make sure"));

  flush();

  while (! Serial.available()) yield();
  
  if (Serial.read() != 'Y') {
    Serial.println("Cancel remove (IP : use capital Y if you want to remove)");
    flush();
    return true;
  }

  // remove
  if (! sd.chdir(RootDir)) {
    Serial.print(F("Error during opening base directory "));
    Serial.println(RootDir);
    return false;
  }
  
  if (dir) {
    if (sd.rmdir(DirContent[sel].name)) Serial.print(F("Removed "));
    else  Serial.print(F("Error during removing: "));   
  }
  else { // removing file
    if (sd.remove(DirContent[sel].name)) Serial.print(F("Removed "));
    else Serial.print(F("Error during removing: "));
  }
  
  Serial.println(DirContent[sel].name);
  
  return(list_files(true,false));
}

/*
 * read file content
 * @param hex : 
 *  false : only display in ASCII
 *  true : display in both HEX and ASCII
 */
bool Read_file(bool hex)
{
   uint8_t offset;
   // display content of current directory
   for(offset = 0; offset < MAX_DIR_ENTRY; offset++){
    
    if (! strlen(DirContent[offset].name)) break;
    
    Serial.print(offset);
    Serial.print("\t");
    
    if (! DirContent[offset].isDir) Serial.println(DirContent[offset].name);
    else Serial.println(F("Directory. can not select"));
  }

  Serial.println(F("Which file NUMBER do you want to select ? q = quit "));

  if (GetEntryNum() == -1) return false;   

  if (sel > offset -1) {
    Serial.println("Invalid selection.");
    return false;
  }
  
  if (DirContent[sel].isDir) {
    Serial.println(F("Not a valid file."));
    return false;
  }
  
  if (! sd.chdir(RootDir)) {
    Serial.print(F("Error during opening base directory "));
    Serial.println(RootDir);
    return false;
  }

  // open file
  if (! sd_file.open(DirContent[sel].name, O_READ))
    {
      Serial.print(F("file.open failed : "));
      Serial.println(DirContent[sel].name);
      return(false);
    }

    uint8_t i = 0;
    char bufhex[40];
    
    while (sd_file.available())
    {
      char ch;
      if (sd_file.read(&ch, 1) == 1) // Read a single char

      if (hex) {
        Serial.print(ch, HEX);
        bufhex[i++] = ch;
        
        if (i > 35) {
          Serial.print("\t");
          
          for(uint8_t j = 0 ; j < i ; j++){
            if (bufhex[j] > 0x1f) Serial.print(bufhex[j]);
            else Serial.print(".");
          }
          Serial.println("\r");
          i = 0;
        }
      }
      else {
        
        if (ch == '\r' || ch == '\n') {
          Serial.print(ch);
          i = 0;
        }
        else {
          if (ch > 0x1f) Serial.print(ch);
          else Serial.print("?");
          Serial.print(" ");
      
          if (i++ > 70) {
            Serial.println("\r");
            i = 0;
          }
        }
      }
    }
    
    // do we need to still display last line ASCII info
    if (hex && i != 0) {
      
      // complete HEX entry
      for(uint8_t j = i ; j != 35 ; j++){
        Serial.print("  ");
      }
      // include spact
      Serial.print("\t");

      // display rest for line
      for(uint8_t j = 0 ; j < i ; j++){
        if (bufhex[j] > 0x1f) Serial.print(bufhex[j]);
        else Serial.print(".");
      }
      
      Serial.println("\r");
      
    }
    sd_file.close();
    return(true);
}

/*
 * move one level up in the directory structure
 */
bool OneLevelUP()
{
  uint16_t i, j;
  
  for (i = 0, j = 0; i < strlen(RootDir); i++) {
    if (RootDir[i] == '/' ) j = i;
  }

  // returning to root
  if (j == 0) return(list_files(true,true));

  // still level(s) directory down
  RootDir[j] = 0x0;
  return(list_files(true,false));   
}

//**********************************************************************************************
// Parts of below routines taken from Sparkfun Artemis Openlog Library (https://github.com/sparkfun/OpenLog_Artemis)
//**********************************************************************************************
void beginSD()
{
#if (defined ARTEMIS_OPENLOG) || (defined MICROMOD_MMDLCB)
  pinMode(PIN_MICROSD_POWER, OUTPUT);
 // pin_config(PinName(PIN_MICROSD_POWER), g_AM_HAL_GPIO_OUTPUT); // Make sure the pin does actually get re-configured
#endif

  // only on ATP
#ifdef ATP
  pinMode(PIN_MICROSD_CD_SELECT, INPUT);
  if (!digitalRead(PIN_MICROSD_CD_SELECT)){
    Serial.println("NO card detected\n");
    online_microSD = false;
    return;   
  }
#endif

  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
 // pin_config(PinName(PIN_MICROSD_CHIP_SELECT), g_AM_HAL_GPIO_OUTPUT); // Make sure the pin does actually get re-configured
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); // Be sure SD is deselected
  
#if (defined ARTEMIS_OPENLOG) || (defined MICROMOD_MMDLCB)
  // For reasons I don't understand, we seem to have to wait for at least 1ms after SPI.begin before we call microSDPowerOn.
  // If you comment the next line, the Artemis resets at microSDPowerOn when beginSD is called from wakeFromSleep...
  // But only on one of my V10 red boards. The second one I have doesn't seem to need the delay!?
  delay(5);

  microSDPowerOn();

  //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
#endif

  if (sd.begin(SD_CONFIG) == false) // Try to begin the SD card using the correct chip select
  {
    Serial.println(F("SD init failed (first attempt). Trying again...\r\n"));
    for (int i = 0; i < 250; i++) //Give SD more time to power up, then try again
    {
      delay(1);
    }
    if (sd.begin(SD_CONFIG) == false) // Try to begin the SD card using the correct chip select
    {
      Serial.println(F("SD init failed (second attempt). Is card present? Formatted?"));
      Serial.println(F("Please ensure the SD card is formatted correctly using https://www.sdcard.org/downloads/formatter/"));
      digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
      online_microSD = false;
      return;
    }
  }

  //Change to root directory. All new file creation will be in root.
  if (sd.chdir() == false)
  {
    Serial.println(F("SD change directory failed"));
    online_microSD = false;
    return;
  }

  online_microSD = true;
}

#if (defined ARTEMIS_OPENLOG) || (defined MICROMOD_MMDLCB)

void microSDPowerOn()
{
# ifdef MICROMOD_MMDLCB
  digitalWrite(PIN_MICROSD_POWER, HIGH);
#endif
# ifdef ARTEMIS_OPENLOG
  digitalWrite(PIN_MICROSD_POWER, LOW);
#endif
}

void microSDPowerOff()
{
# ifdef MICROMOD_MMDLCB
  digitalWrite(PIN_MICROSD_POWER, LOW);
#endif
# ifdef ARTEMIS_OPENLOG
  digitalWrite(PIN_MICROSD_POWER, HIGH);
#endif
}

#endif //(defined ARTEMIS_OPENLOG) || (defined MICROMOD_MMDLCB)

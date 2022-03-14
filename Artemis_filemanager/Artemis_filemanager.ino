/**
 *  
 * A filemanager for reading and writing the MicroSD on different Artemis platforms :
 * 
 * 1. MicroMod MainBoard (DEV-18576) with MM Artemis Processor (WRL-16781)
 * 2. MicroMod Data logger carrier board (DEV-16829) with MM Artemis Processor (WRL-16781)
 * 3. Artemis OpenLog (DEV-16832)
 * 4. Artemis ATP (DEV-15442) with external SparkFun Level Shifting microSD (DEV-13743)
 * 
 * Parts of the code has been taken from the OpenLog sketch/program created by Sparkfun
 * 
 * Author : paulvha@hotmail.com
 * 
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // MAKE SURE TO SELECT THE RIGHT BOARD IN THE SKETCH BELOW AND IN ARDUINO IDE -> TOOLS -> BOARD !
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * 
 * ============================ Versioning ==============================================
 *  1.4                                 / March 2022
 *  Added abbreviation input posibility
 *  Added rename file function
 *  Improved HEX display
 *  Fixed some typo's
 *  
 *  1.3                                 / February 2022
 *  Added option 11 to show hidden folder / files  
 *  Fixed display size
 *  
 *  1.2                                 / February 2022
 *  Added support for Micromod datalogger & MicroMod main board
 *  
 *  1.1                                 / December 2021
 *  Added support for ATP
 *  
 *  1.0                                 / December 2021
 *  initial version for Aretemis OpenLog 
 * 
 * =============================== Connections ===========================================
 * 
 * External   Artemis
 * MicroSD    ATP
 * VCC        3V3 
 * D2/CS      23  (defined by PIN_MICROSD_CHIP_SELECT)
 * D1         MOSI / COPI
 * SCK        SCK
 * D0         MISO / CIPO
 * CD         27  (defined by PIN_MICROSD_CD_SELECT) Note1
 * GND        GND
 * 
 * Note1: set PIN_MICROSD_CD_SELECT to zero to disable 
 * 
 *            Artemis
 * MicroSD    MICROMOD_MMDLCB  (FYI only fixed)
 * D2/CS      41 (on-board) or 16 (on HEADER_CS) (defined by PIN_MICROSD_CHIP_SELECT)
 * D1         MOSI / COPI
 * SCK        SCK
 * D0         MISO / CIPO
 * CD         Not connected (on-board) / not used (on HEADER_CS)
 * GND        GND
 * POWER      33  (define by PIN_MICROSD_POWER)
 * 
 *            Artemis
 * MicroSD    MICROMOD_MAIN board  (FYI only fixed)
 * D2/CS      28
 * D1         MOSI / COPI
 * SCK        SCK
 * D0         MISO / CIPO
 * CD         Not connected
 * GND        GND 
 *
 *            Artemis
 * MicroSD    OpenLog board  (FYI only fixed)
 * D2/CS      23
 * D1         MOSI / COPI
 * SCK        SCK
 * D0         MISO / CIPO
 * CD         Not connected
 * GND        GND 
 * POWER      15
 * 
 *  ================================ Disclaimer ======================================
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 *  The above is more to be covered in case you loose data. Usage is at your own risk
 *  ===================================================================================
 */

#include <SPI.h>
#include <SdFat.h> //SdFat v2.0.7 by Bill Greiman: http://librarymanager/All#SdFat_exFAT

/*********************************************************************************
 Define whether this is used on Artemis OpenLog, MICROMOD_MMDLCB, MAINB or ATP
 On OpenLog & Micromod MMDLCB we need to switch on the MicroSD power
 On ATP the external microSD has (optional) CD detect function
 on Micromod Main board, no CD, NO power switch */
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// MAKE SURE TO SELECT THE RIGHT BOARD HERE AND IN ARDUINO IDE -> TOOLS -> BOARD
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ================================= Board Selection ========================================
//#define ARTEMIS_OPENLOG                  // select Artemis Openlog board
//#define MICROMOD_MMDLCB                  // Micromod data logger carrier board
#define MICROMOD_MAINB                   // Micromod mainboard
//#define ATP                              // ATP with external microSD

// =================================== Global Defines =====================================
// SD_FAT_TYPE = 0 for SdFat/File, 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 3 

// needed to display directory
#define MAXFILENAME 250                     // maximum length file or directory name on exFAT
#define MAX_DIR_ENTRY 100                   // maximum entries in a current directory

// ================================= Board definitions =======================================
#ifdef MICROMOD_MMDLCB
const byte PIN_MICROSD_POWER = 33;          // LDO power
const int sdPowerDownDelay = 100;           // Delay for this many ms before turning off the SD card power
const byte PIN_MICROSD_CHIP_SELECT = 41;    // CS (chip select IOM)
#endif // MICROMOD_MMDLCB

#ifdef MICROMOD_MAINB
const byte PIN_MICROSD_CHIP_SELECT = 28;    // CS (chip select IOM)
#endif // MICROMOD_MAINB

#ifdef ARTEMIS_OPENLOG
const byte PIN_MICROSD_POWER = 15;          // power the FET
const int sdPowerDownDelay = 100;           // Delay for this many ms before turning off the SD card power
const byte PIN_MICROSD_CHIP_SELECT = 23;    // CS (chip select)
#endif // ARTEMIS_OPENLOG

#ifdef ATP
const byte PIN_MICROSD_CD_SELECT = 27;      // Connect CD here (card detect) set as 0 -zero- to disable
const byte PIN_MICROSD_CHIP_SELECT = 23;    // Connect CS here (chip select)
#endif // ATP

//*******************************************************************************************
//***                    NO NEED TO MAKE CHANGES BEYOND THIS POINT                         **
//*******************************************************************************************

// =================================== Global variables =====================================

#define SD_CONFIG SdSpiConfig(PIN_MICROSD_CHIP_SELECT, SHARED_SPI, SD_SCK_MHZ(24)) // 24MHz

#if SD_FAT_TYPE == 1
SdFat32 sd;
File32 sd_root; //current dir
File32 sd_file; //current directory entry
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile sd_root; //current dir
ExFile sd_file; //current directory entry
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile sd_root; //current dir
FsFile sd_file; //current directory entryto
#else // SD_FAT_TYPE == 0
SdFat sd;
File sd_root; //current dir
File sd_file; //current directory entry
#endif  // SD_FAT_TYPE

struct entry{
    char name[MAXFILENAME];  // name of directory entry
    bool isDir;              // true if directory
};

struct entry DirContent[MAX_DIR_ENTRY];
char fname[MAXFILENAME];

// needed to go directory levels down and up
char RootDir[MAX_DIR_ENTRY * 5];
char TempDir[MAX_DIR_ENTRY * 5];

bool ShowHidden = false;    // show hidden folders / files
bool online_microSD = false;
int  sel, offset;
uint16_t pdate, ptime;      // get date and time
bool Valid_entry = false;   // was valid entry was shown during DisplayArray

// ======================= start of program ===================================

void setup() {

  Serial.begin(115200); 
  while (!Serial); 

  Serial.println(F("Artemis MicroSD filemanager (Version 1.4)\nPress Enter to continue."));
  GetFileName();
  
  beginSD();

  if (!online_microSD) {
    Serial.println(F("Something went wrong during initializing MicroSD."));
    Serial.println(F("Is the MicroSD installed correctly ?"));
    Serial.println(F("Is the right board selected in top of sketch ?"));
    Serial.println(F("Is the right board selected in the Arduino IDE ?"));
    Serial.println(F("Freeze !"));
    while(1);
  }

  // init list
  DirContent[0].name[0] = 0x0;

  Serial.println(F("Loading data from MicroSD"));

  // get first file list
  list_files(false,true);
}

void loop() {
  Serial.print("\ncurrent directory :");
  Serial.println(RootDir);
  Serial.println(F("\n\n1  = list the files in current directory"));
  Serial.println(F("2  = Change directory"));
  Serial.println(F("3  = Create new directory"));
  Serial.println(F("4  = Create new file"));
  Serial.println(F("5  = Back to root directory"));
  Serial.println(F("6  = Remove file"));
  Serial.println(F("7  = Remove directory"));
  Serial.println(F("8  = One level up to parent directory"));
  Serial.println(F("9  = Read file. Display in ASCII only"));
  Serial.println(F("10 = Read file. Display in HEX and ASCII"));

  if (ShowHidden) Serial.println(F("11 = Do NOT show hidden folder / files"));
  else Serial.println(F("11 = Show hidden folder / files"));

  Serial.println(F("12 = Rename file"));
  Serial.println(F("15 = Close SD"));
  Serial.print(F("\nYour input (? = help)") );

  if (GetEntryNum(true) == -1) return;
  
  switch (sel) {
    case 0:   // ignore zero
      break;  
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
      Read_entry(false);
      break;
    case 10:
      Read_entry(true);
      break;
    case 11:
      ShowHidden = !ShowHidden;
      break;
    case 12:
      Rename_file();
      break;
   
    case 15:
      digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); // Make sure microSD is deselected
      
#if (defined ARTEMIS_OPENLOG) || (defined MICROMOD_MMDLCB)
      delay(sdPowerDownDelay);
      microSDPowerOff();
#endif

      Serial.println(F("Press reset to restart"));
      while(1);      
      break;
    default:
      Serial.print(F("Invalid input 0x"));
      Serial.println(sel, HEX);
  }
}

/* read an entry number 
 *   
 * return
 *  sel     result is also stored in global variable sel.
 *  -1      invalid input or quit
 *  
 *  @parameter ext = true will also allow input as : 
 *  ls = list                  => 1
 *  cd = change directory      => 2
 *  ro = return to root        => 5
 *  rf = remove file           => 6
 *  rd = remove directory      => 7 
 *  up = one level up          => 8
 *  ad = display ASCII         => 9
 *  hd = display hex and ASCII => 10
 *  rn = rename file           => 12
 */
int GetEntryNum(bool ext)
{
  flush();
  char ABR_input = '0';

  // wait for input
  while (!Serial.available());
  char c = '0';
  sel = 0;
   
  while (c != '\r' && c != '\n') {

    if (c == 'q' || c == 'Q') return(-1);
    
    // allow extended input
    if (ext) {
      
      if (c == 'l'|| c == 'c'|| c == 'r'|| c == 'u' ||c == 'a'|| c == 'h') 
        ABR_input = c;

      else if (c == 's' &&  ABR_input == 'l') // list
        sel = 1;

      else if (c == 'd' &&  ABR_input == 'c') // change directory
        sel = 2;

      else if (c == 'o' &&  ABR_input == 'r') // change to root directory
        sel = 5;
        
      else if (c == 'p' &&  ABR_input == 'u') // move to parent directory
        sel = 8;

      else if (c == 'd' &&  ABR_input == 'a') // display file in ASCII
        sel = 9;

      else if (c == 'd' &&  ABR_input == 'h') // display file content in HEX and ASCII
        sel = 10;
        
      else if (c == 'f' &&  ABR_input == 'r') // remove file
        sel = 6;
        
      else if (c == 'n' &&  ABR_input == 'r') // rename file
        sel = 12;
        
      else if (c == 'd' &&  ABR_input == 'r') // remove directory
        sel = 7;
        
      else
        ABR_input = '0';

      if (sel != 0 && ABR_input != '0') goto Idone;
    }
    
    // number input
    if (ABR_input == '0') {
    
      if (c >= '0' && c <= '9') {
        sel = sel * 10;
        sel = sel + (c - '0');
      }
      else 
         goto invalid; 
    }
 
    delay(10);
    c = Serial.read();
    
    if (c == '?') {
      DisplayOptions(ext);
      ABR_input = c = '0';
      sel = 0;
    }
    else
      Serial.print(c);
  }
Idone:
  Serial.println("\r");
  return(sel);

invalid:
  Serial.print("\nInvalid number 0x");
  Serial.println(c, HEX);
  return(-1);
}

/*
 * display the options for entry
 */
void DisplayOptions(bool ext)
{
  Serial.println("\nEnter a number");
  if (! ext) return;
  Serial.println("or\nls\tlist directory");
  Serial.println("cd\tChange directory");
  Serial.println("ro\tChange to root directory");
  Serial.println("rf\tRemove file");
  Serial.println("rd\tRemove directory");
  Serial.println("up\tChange parent directory of current");
  Serial.println("ad\tDisplay file content in ASCII");
  Serial.println("hd\tDisplay file content in HEX and ASCII");
  Serial.println("rn\tRename file");
}

/*
 * read a filename and store in global variable fname
 * @return number of characters in fname
 */
int GetFileName()
{
  flush();
  offset = 0;
  
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
  bool NoEntryDetected = true;
     
  if (root) sprintf(RootDir,"%s","/");  
  
  if (!sd.chdir("/")) {
    Serial.println(F("Error during setting start position\n"));
    return false;    
  }
  
  if (!sd_root.open(RootDir)) {
    Serial.printf("Error: during opening SD card %s\n",RootDir);
    return false;
  }

  if (!sd_root.isDir()) {
    Serial.printf("Error: %s is NOT a directory\n\n",RootDir);
    sd_root.close();
    return false;
  }

  sd_root.rewind();
  
  if (show) Serial.printf("Content of %s\n", RootDir);
  
  while (sd_file.openNext(&sd_root, O_RDONLY))
  {
    // skip hidden entry (e.g. volume directory)
    if (sd_file.isHidden()) {
      if (! ShowHidden) {
        sd_file.close();
        continue;
      }
    }

    // get name and size
    if (sd_file.getName(fname, MAXFILENAME)) {

      NoEntryDetected = false;
      
      sprintf(DirContent[offset].name,"%s",fname);      // add entry in array
      DirContent[offset].isDir = sd_file.isDir();
    
      if (show) {
       
        Serial.print(offset);
        if (sd_file.isDir()) Serial.print("\tdir \t");
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
        
        Serial.print("\tsize\t");
        uint32_t fs = sd_file.fileSize();

        if (fs > 0)  Serial.print(fs);
        else Serial.print("0   ");

        if (sd_file.isHidden()) {
          if (ShowHidden) Serial.printf("\t%s (hidden)\n",fname);
          else Serial.printf("\t%s\n",fname);
        }
        else
          Serial.printf("\t%s\n",fname);
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
  
  if(show & NoEntryDetected) {
    Serial.println("Empty directory\r");
  }
  
  sd_root.close();
  return(true);
}

/*
 * place the MicroSD correctly in the current Base directory
 */
bool Set_dir()
{
  if (! sd.chdir("/")) {
    Serial.printf("Error during opening root directory.\n");
    return false;
  }

  if (strlen(RootDir) == 1) return true;

  if (! sd.chdir(RootDir)) {
    Serial.printf("Error during opening base directory %s.\n", RootDir);
    return false;
  }
  
  return true;
}

/*
 * change to a next level directory
 */
bool change_dir()
{
  offset = DisplayArray(false);

  if (! Valid_entry) {
    Serial.println(F("NO valid directory found."));
    return false;
  }
  
  Serial.print(F("Which directory NUMBER do you want to select ? q = quit "));
  
  if (GetEntryNum(false) == -1) return false;
  
  if (sel > offset -1) {
    Serial.println(F("Invalid selection."));
    return false;
  }
  
  if (! DirContent[sel].isDir) {
    Serial.println(F("Not a valid directory."));
    return false;
  }

  sprintf(TempDir,"%s%s/", RootDir, DirContent[sel].name);
    
  sprintf(RootDir, "%s", TempDir);
 
  return(list_files(true,false));
}

/*
 * create a new file or directory in the current directory
 */
bool create_file(bool dir)
{
  offset = 0 ;

  if (dir)
    Serial.print(F("Enter the name of the directory to create. Only enter is abort. "));
  else
    Serial.print(F("Enter the name of the file to create. Only enter is abort. "));

  if (GetFileName() == 0) return true;

  if (! Set_dir()) return false;

  // check it exists ??
  if (sd_file.exists(fname))
  {
    Serial.printf("%s already exists.\n", fname);
    sd_file.close();
    return false;
  } 
  
  sd_file.close();

  if(dir) {
    if ( ! sd.mkdir(fname, false)) {
      Serial.println(F("Error during creating directory."));
    }
    else {
      Serial.println(F("Directory has been created."));
      sd_file.close();
    }
  }
  else  // file
  {
    
    if (! sd_file.open(fname, O_CREAT | O_APPEND | O_WRITE))
    {
      Serial.println(F("Failed to create file."));
    }
    else {
      Serial.println(F("File has been created."));

      Serial.printf("Which sentence to do you want to include in the file %s. Only enter is nothing.\n",fname);

      if (GetFileName() != 0) {
      
        Serial.print(F("How often to repeat this sentence ? (0 = cancel) "));
         
        if (GetEntryNum(false) == -1 || sel == 0) {
          sd_file.close();
          return true;
        }

        for (uint8_t b = 0; b < sel ; b++) sd_file.println(fname);
      }
      
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
  
  // display directory content
  if (dir)  offset = DisplayArray(false);
  else      offset = DisplayArray(true);

  if (! Valid_entry) {
    Serial.println(F("NO valid entry found"));
    return false;
  }
  
  if (dir)
    Serial.println(F("Which directory NUMBER do you want to select ? q = quit "));
  else
    Serial.println(F("Which file NUMBER do you want to select ? q = quit "));
  
  if (GetEntryNum(false) == -1) return false;
    
  if (sel > offset -1) {
    Serial.println("Invalid selection.");
    return false;
  }

  if (dir) {
    if (! DirContent[sel].isDir) {
      Serial.printf("%s Not a valid directory.\n",DirContent[sel].name);
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
      Serial.printf("ERROR. can not remove %s as it has %d entry(s)\n", DirContent[sel].name, count);
      return false;
    }
    else {
      Serial.printf("%s empty directory\n",DirContent[sel].name);
    }
  }
  else {
  
    if (DirContent[sel].isDir) {
      Serial.printf("%s Not a valid file.\n",DirContent[sel].name);
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
    Serial.println(F("Cancel remove (TIP : use capital Y if you want to remove)"));
    flush();
    return true;
  }

  if (! Set_dir()) return false;

  // remove 
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
bool Read_entry(bool hex)
{

  // display content of current directory
  offset = DisplayArray(true);
  
  if (! Valid_entry) {
    Serial.println(F("NO valid entry found"));
    return false;
  }
  
  Serial.println(F("Which file NUMBER do you want to select for reading ? q = quit "));

  if (GetEntryNum(false) == -1) return false;   

  if (sel > offset -1) {
    Serial.println(F("Invalid selection."));
    return false;
  }

  if (DirContent[sel].isDir) {
    Serial.println(F("Not a valid file."));
    return false;
  }

  if (! Set_dir()) return false;

  // open file
  if (! sd_file.open(DirContent[sel].name, O_READ))
  {
    Serial.printf("Error: could not open : %s\n",DirContent[sel].name);
    return false;
  }

  uint32_t fs = sd_file.fileSize();
  if (fs == 0) {
     Serial.printf("%s is empty.\n\n",DirContent[sel].name);
     return true;
  }
  else  
    Serial.printf("Content of %s (%d bytes)\n\n",DirContent[sel].name, fs);
  
  uint8_t j, i = 0;
  char ch, bufhex[40];
  
  while (sd_file.available())
  {
    if (sd_file.read(&ch, 1) == 1) // Read a single character

    if (hex) {

      // add leading zero on Hex if lower than 0x10
      if (ch < 0x10) Serial.print("0");

      // display and add character
      Serial.print(ch, HEX);
      bufhex[i++] = ch;
      
      Serial.print(" ");
  
      if (i == 30) {             // after 30 characters  
        
        for (j = i ; j < 40; j++) Serial.print(" ");
        
        for (j = 0 ; j < i  ; j++){
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
    for(j = i ; j < 30 ; j++)  Serial.print("   ");  // 3 spaces each entry
    
    // include space
    for (j ; j < 40; j++) Serial.print(" ");

    // display rest for line
    for(j = 0 ; j < i ; j++){
      if (bufhex[j] > 0x1f) Serial.print(bufhex[j]);
      else Serial.print(".");
    }
    
    Serial.println("\r");
    
  }
  sd_file.close();
  return(true);
}

/*
 * rename an existing file
 */
bool Rename_file()
{
  // display file content of current directory
  offset = DisplayArray(true);
  
  if (! Valid_entry) {
    Serial.println(F("NO valid file entry found."));
    return false;
  }
  
  Serial.println(F("Which file NUMBER do you want to select for renaming? q = quit "));

  if (GetEntryNum(false) == -1) return false;   

  if (sel > offset -1) {
    Serial.println(F("Invalid selection."));
    return false;
  }
  
  if (DirContent[sel].isDir) {
    Serial.println(F("Not a valid file."));
    return false;
  }
  
  Serial.print("Selected: ");
  Serial.println(DirContent[sel].name);

  Serial.print(F("Enter the new name of the file. Only enter is abort. "));

  if (GetFileName() == 0) return true;

  if (! Set_dir()) return false;
  
  // check it exists ??
  if (sd_file.exists(fname))
  {
    Serial.printf("%s already exists\n", fname);
    sd_file.close();
    return false;
  } 
  
  sd_file.close();

  // open file
  if (! sd_file.open(DirContent[sel].name, O_WRONLY))
  {
    Serial.printf("Error: Could not open: %s\n", DirContent[sel].name);
    return false;
  }

  // rename the file
  if (!sd_file.rename(fname)) {
    Serial.print(F("Rename failed."));
    sd_file.close();
    return false;
  }

  // close
  sd_file.close();

  return true;
}

/*
 * display directory / array content
 * @parameter CheckFile
 *  true  : check this is a file
 *  false : check this is a directory
 *  
 *  @return
 *  number of entries in current directory
 */
uint8_t DisplayArray(bool CheckFile)
{
  Valid_entry = false;
  
  // display content of current directory
  for (offset = 0; offset < MAX_DIR_ENTRY; offset++) {
    
    // end if array entries
    if (! strlen(DirContent[offset].name)) break;
    
    Serial.print(offset);
    Serial.print("\t");

    if (CheckFile) {
      if (! DirContent[offset].isDir) {
        Valid_entry = true;
        Serial.println(DirContent[offset].name);
      }
      else Serial.println(F("\tDirectory. can not select"));
    }
    else {
      if ( DirContent[offset].isDir){
        Valid_entry = true;
        Serial.println(DirContent[offset].name);
      }
      else Serial.println(F("\tFile. can not select"));
    }
  }
  
  return(offset);
}

/*
 * move one level up in the directory structure
 */
bool OneLevelUP()
{
  uint16_t i, j, s;
  // look for the last entry
  for (i = 0, j = 0; i < strlen(RootDir); i++) {
    if (RootDir[i] == '/' ) {
      s = j;  // second last
      j = i;  // last
    }
  }

  // returning to root
  if (j == 0) return(list_files(true,true)); 

  // still level(s) directory down
  RootDir[s+1] = 0x0;
  return(list_files(true,false));   
}

//**********************************************************************************************
// Parts of below routines taken from Sparkfun Artemis Openlog Library 
// (https://github.com/sparkfun/OpenLog_Artemis)
//**********************************************************************************************
void beginSD()
{
#if (defined ARTEMIS_OPENLOG) || (defined MICROMOD_MMDLCB)
  pinMode(PIN_MICROSD_POWER, OUTPUT);
#endif

  // only on ATP
#ifdef ATP
  if (PIN_MICROSD_CD_SELECT > 0) {
    pinMode(PIN_MICROSD_CD_SELECT, INPUT);
    if (!digitalRead(PIN_MICROSD_CD_SELECT)){
      Serial.println("NO card detected\n");
      online_microSD = false;
      return;   
    }
  }
#endif

  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
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
      Serial.println(F("SD init failed (second attempt).\n\tIs card present?\n\tFormatted ?\n\tRight board (both in sketch and IDE) selected during compile ?"));
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

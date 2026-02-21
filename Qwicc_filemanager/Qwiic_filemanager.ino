/*
  QWIIC OpenLog filemanager

  This sketch allows you to access the SD card on a Sparkfun QWIIC Openlog module (DEV-15164).
  It has been tested on Artemis/Apollo3 V1.2.3 and library version 2.2.1.
  It has also be tested on ATmega 2560 (V1.2), SAMD51D Thing plus, an Arduino Uno (V1.3) an
  ESP32 (V1.4) and Qwiic Micro - SAMD21 (V1.5)
  Arduino UNO R4 Wifi (V1.6)

  By: Paulvha@hotmail.com
  initial date: March, 2022
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  =========================== Disclaimer ==============================
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  =========================== Versioning ==============================

  V1.6                             / february 2026
  added setting WIREPORT
  added setting setWireTimeout for UNO R4 to handle clock stretch

  V1.5                             / January 2023
  Update to connect to SparkFun Qwiic Micro - SAMD21 Development Board (DEV-15423). 
  Added SERIALPORT as the Serialport needs to be set to SerialUSB instead of Serial.
   
  V1.4                             / January 2023
  Update as tested on Sparkfun ESP32 THING. Do not enable 'SMALLFOOTPRINT'
  Wire on ESP32 does not support clock-stretch. A special 'Softwire' port from the ESP8266
  needs to be in the src directory to support clock stretch.

  QWIIC     ESP32
  Red       3v3
  Black     GND
  Blue      22 / SDA
  Yellow    21 / SCL

  V1.3                             / November 2022
  updated to add SMALLFOOTPRINT so it works on an Arduino UNO as well.

  QWIIC     UNO
  Red       3v3
  Black     GND
  Blue      A4 / SDA
  Yellow    A5 / SCL

  MAKE SURE TO UNCOMMENT THE LINE  '#define SMALLFOOTPRINT 1' In user defines below.

  V1.2                                / September 2022
  Updated the approach to get the directory entries after discovering the issue in Artemis V2.xx Wire-library

  V1.1                                / September 2022
  Changed some the subroutines for enhancements and a better flow
  Made adjustments so the program now also works with Sparkfun QWIIC Openlog module (DEV-15164) connected to an ATmega

  QWIIC    ATMEGA
  Red       3v3
  Black     GND
  Blue      SDA
  Yellow    SCL

  V1.0                                / March 2022
  Initial version
  This has been tested on a Sparkfun QWIIC Openlog module (DEV-15164) connected to an Artemis ATP.
*/

// included in SparkFun_Qwiic_OpenLog_Arduino_Library.h
#include "SparkFun_Qwiic_OpenLog_Arduino_Library.h"

//======================== user defines ===============================

// In case of an MCU board with small memory(like UNO) uncomment
// in default setup the SMALLFOOTPRINT is using 50% of program storage / 75% of dynamic memory
//#define SMALLFOOTPRINT 1

#ifdef SMALLFOOTPRINT  // like UNO or Arduino Nano

#define MAXFILENAME   14                    // maximum length file or directory name (size is hardcoded in Openlog source, do not change)
#define MAX_DIR_ENTRY 30                    // maximum entries in a current directory
#define BUFLEN        300                   // max length to read content (adjust to your board capacity)
#define SUBLEVELS     4                     // the number of directory to go "down" with change directory

#else // ATMEGA / Artemis boards / ESP32 / SAMD / UNO R4

#define MAXFILENAME   14                    // maximum length file or directory name (size is hardcoded in Openlog source)
#define MAX_DIR_ENTRY 100                   // maximum entries in a current directory
#define BUFLEN        3000                  // max length to read content (adjust to your board capacity)
#define SUBLEVELS     5                     // the number of directory to go "down" with change directory

#endif

//===================== Global Variables ==============================

OpenLog myLog;                              // Create instance

#define ROOT myLog.changeDirectory((String) "..");

char buf[BUFLEN];                           // general buffer
int  sel, offset;

struct entry{
    char name[MAXFILENAME];                 // name of directory entry
    bool isDir;                             // true if directory
};

struct entry DirContent[MAX_DIR_ENTRY];     // array to store a directory content
char fname[MAXFILENAME];

// needed to go directory levels down and up
// increase if many sub-directories and/or long names
char RootDir[MAX_DIR_ENTRY * SUBLEVELS];
char TempDir[MAX_DIR_ENTRY * SUBLEVELS];

bool Valid_entry = false;                   // was valid entry was shown during DisplayArray
//============================ Start of program ===============================

#define SERIALPORT Serial
#define WIREPORT Wire1

void setup()
{
  SERIALPORT.begin(115200);
  while (!Serial);
  SERIALPORT.println(F("QWIIC OpenLog filemanager. (V1.6)\nPress Enter to continue."));
  GetFileName();

  WIREPORT.begin();
  WIREPORT.setClock(100000);

#if defined (ARDUINO_UNOR4_WIFI)
  // needed to handle clock stretch on Arduino Uno R4  februari 2026 - V1.6
  WIREPORT.setWireTimeout(100000);
#endif  
  
  if ( !myLog.begin(42, WIREPORT)){                   // Open connection to QWIIC OpenLog
    SERIALPORT.println(F("QWIIC OpenLog filemanager could not be initialised. freeze."));
    while(1);
  }

  // read root directory into buffer
  list_files(false, true);
}

void loop()
{
  SERIALPORT.print(F("\nCurrent directory :"));
  SERIALPORT.println(RootDir);
  SERIALPORT.println(F("1  = list the files in current directory"));
  SERIALPORT.println(F("2  = Change directory"));
  SERIALPORT.println(F("3  = Create new directory"));
  SERIALPORT.println(F("4  = Create new file"));
  SERIALPORT.println(F("5  = Back to root directory"));
  SERIALPORT.println(F("6  = Remove file"));
  SERIALPORT.println(F("7  = Remove directory"));
  SERIALPORT.println(F("8  = One level up to parent directory"));
  SERIALPORT.println(F("9  = Read file. Display in ASCII only"));
  SERIALPORT.println(F("10 = Read file. Display in HEX and ASCII"));
  SERIALPORT.println(F("11 = Add content to file"));
  SERIALPORT.println(F("15 = Close SD"));
  SERIALPORT.print(F("\nYour input (? = help)") );

  // blocking input
  if (GetEntryNum(true) == -1) return;

  switch (sel) {
     case 0:   // ignore zero
      break;
    case 1:
      list_files(true, false);
      break;
    case 2:
      change_dir(0);
      break;
    case 3:
      create_entry(true);
      break;
    case 4:
      create_entry(false);
      break;
    case 5:
      list_files(true, true);
      break;
    case 6:
      remove_entry(false);
      break;
    case 7:
      remove_entry(true);
      break;
    case 8:
      OneLevelUP(true);
      break;
    case 9:
      Read_file(false);
      break;
    case 10:
      Read_file(true);
      break;
    case 11:
      Append_file();
      break;
    case 15:
      myLog.syncFile();
      SERIALPORT.println(F("Bye. Have a great day.\nPress 'reset' to restart."));
      while(1);
      break;

    default:
      SERIALPORT.print(F("Invalid input 0x"));
      SERIALPORT.println(sel, HEX);
  }
}

/* Read an entry number
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
 *  ad = display ascii         => 9
 *  hd = display hex and ascii => 10
 *  ap = append to file        => 11
 */
int GetEntryNum(bool ext)
{
  flush();
  char ABR_input = '0';

  // wait for input
  while (!SERIALPORT.available());
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

      else if (c == 'd' &&  ABR_input == 'r') // remove directory
        sel = 7;

      else if (c == 'p' &&  ABR_input == 'a') // Append file
        sel = 11;

      else
        ABR_input = '0';

      if (sel != 0 && ABR_input != '0') goto idone;
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
    c = SERIALPORT.read();

    if (c == '?') {
      DisplayOptions(ext);
      ABR_input = c = '0';
      sel = 0;
    }
    else
      SERIALPORT.print(c);
  }

idone:
  SERIALPORT.println();
  return(sel);

invalid:
  SERIALPORT.print(F("\nInvalid number 0x"));
  SERIALPORT.println(c, HEX);
  return(-1);
}

/*
 * display the options for entry
 */
void DisplayOptions(bool ext)
{
  SERIALPORT.println(F("\nEnter a number from the menu selection"));
  if (! ext) return;
  SERIALPORT.println(F("\t\tOR\nls\tlist directory"));
  SERIALPORT.println(F("cd\tChange directory"));
  SERIALPORT.println(F("ro\tChange to root directory"));
  SERIALPORT.println(F("rf\tRemove file"));
  SERIALPORT.println(F("rd\tRemove directory"));
  SERIALPORT.println(F("up\tChange to parent directory of current"));
  SERIALPORT.println(F("ad\tDisplay file content in ASCII"));
  SERIALPORT.println(F("hd\tDisplay file content in HEX and ASCII"));
  SERIALPORT.println(F("ap\tAppend test to existing or new file."));
}

/*
 * read a filename and store in global variable fname
 * @ return number of characters in fname
 */
uint8_t GetFileName()
{
  flush();
  offset = 0;

  // wait for input
  while(!SERIALPORT.available()) yield();
  char c = SERIALPORT.read();

  while(c != '\r' && c != '\n' && offset < MAXFILENAME -1) {
    fname[offset++] = c;
    SERIALPORT.print(c);
    while (! SERIALPORT.available()) yield();
    c = SERIALPORT.read();
  }

  SERIALPORT.println();

  fname[offset] = 0x0;

  return(offset);
}

/*
 * flush any pending input
 */
void flush()
{
  delay(100);

  while (SERIALPORT.available()) {
    SERIALPORT.read();
    delay(50);
  }
}

/*
 * list a directory content
 *
 * @param show :
 *  true : show the detected directory content
 *  false : only store the content
 *
 * @param root :
 *  true  : start of root directory
 *  false : start at current directory
 *
 * @return
 *  return true if all went well, false if there was an error/warning
 *
 */
bool list_files(bool show,bool root)
{
  offset = 0;                     // where to store next directory entry
  bool NoEntryDetected = true;    // detect empty directory
  bool ret = true;                // return true if all went well, false if there was an error/warning

  if (root) {
    ROOT;                         // rewind to root
    sprintf(RootDir,"%s","/");
  }

  // empty wire buffer, needed for Wire on V2.x.x (bug) to remove pending Wire buffer
  while (Wire.available()) Wire.read();

  myLog.searchDirectory("*");

  String fileName = myLog.getNextDirectoryItem();

  // first get all the names
  while (fileName != "") //getNextDirectoryItem() will return "" when we've hit the end of the directory
  {
    // at least ONE directory entry has been detected
    NoEntryDetected = false;

    uint8_t filelength = fileName.length();

    // remove '/' which was added to indicate directory
    if (fileName[filelength -1] == '/'){
      DirContent[offset].isDir = true;
      filelength--;
    }
    else {
      DirContent[offset].isDir = false;
    }

    if (filelength > MAXFILENAME -1) {
      SERIALPORT.println(F("WARNING : Directory entry name to long. Truncating\n"));
      ret = false;
      filelength = MAXFILENAME -1;
    }

    // add entry in array
    for (uint8_t i = 0 ; i != filelength; i++) {
      DirContent[offset].name[i] = fileName[i];
      DirContent[offset].name[i + 1] = 0x0;
    }

    offset++;

    if (offset == MAX_DIR_ENTRY) {
      SERIALPORT.println(F("WARNING : can not add more entrys in array. skipping\n"));
      ret = false;
      offset--;
    }

    fileName = myLog.getNextDirectoryItem();
  }

  // terminate last entry
  DirContent[offset].name[0] = 0x0;

  // display information
  if (show) {

    if ( NoEntryDetected) {
      SERIALPORT.println(F("Empty directory\r"));
    }
    else {
      SERIALPORT.print(F("\nContent for "));
      SERIALPORT.println(RootDir);

      for (uint8_t ind = 0; ind < offset; ind++) {

        SERIALPORT.print(DirContent[ind].name);

        // add enough spaces to align output
        for (uint8_t n = strlen(DirContent[ind].name) ; n <= 20; n++) SERIALPORT.print(" ");

        if(DirContent[ind].isDir) {
          SERIALPORT.println("DIR \t--");
        }
        else {
          SERIALPORT.print("FILE\t");

          // Get size of file
          int32_t sizeOfFile = myLog.size(DirContent[ind].name);
          SERIALPORT.println(sizeOfFile);
        }
      }
    }
  }

  return ret;
}

/*
 * change to a sub-directory
 * @parameter ind : offset in array is already known
 *
 * @return
 *  true : if all went OK
 *  false: in case of error or cancel
 */
bool change_dir(uint8_t ind)
{
  if (ind == 0) {

    // display directories
    offset = DisplayArray(false);

    if (! Valid_entry) {
      SERIALPORT.println(F("NO valid directory found."));
      return false;
    }

    SERIALPORT.println(F("Which directory NUMBER do you want to select ? q = quit "));

    if (GetEntryNum(false) == -1) return false;

    if (sel > offset -1) {
      SERIALPORT.println(F("Invalid selection."));
      return false;
    }
  }
  else
    sel = ind;

  if (! DirContent[sel].isDir) {
    SERIALPORT.println(F("Not a valid directory."));
    return false;
  }

  // move in directory
  myLog.changeDirectory(DirContent[sel].name);

  sprintf(TempDir,"%s%s/", RootDir, DirContent[sel].name);
  sprintf(RootDir, "%s", TempDir);

  return(list_files(true,false));
}

/*
 * create a new file or directory in the current directory
 * @parameter dir : true is create a directory else file
 *
 * @return
 *  true : if all went OK
 *  false: in case of error or cancel
 */
bool create_entry(bool dir)
{
  offset = 0 ;

  if (dir)
    SERIALPORT.print(F("Enter the name of the directory to create. Only enter is abort. "));
  else
    SERIALPORT.print(F("Enter the name of the file to create. Only enter is abort. "));

  if (GetFileName() == 0) return true;

  // check it exists
  long sizeOfFile = myLog.size(fname);

  if (sizeOfFile > -1) {
    SERIALPORT.println(F("Already exists"));
    return false;
  }

  if (dir) {
    if ( ! myLog.makeDirectory((String) fname)) {
      SERIALPORT.println(F("Error during creating directory."));
    }
    else {
      SERIALPORT.println(F("Directory has been created."));
    }
  }
  else  // file
  {
    if ( ! myLog.create((String) fname)) {
      SERIALPORT.println(F("Failed to create file."));
    }
    else {
      SERIALPORT.println(F("File has been created."));
    }
  }

  return(list_files(true,false));
}

/*
 * remove file or directory
 * @param dir : true for directory
 *
 * @return
 *  true : if all went OK
 *  false: in case of error or cancel
 */
bool remove_entry(bool dir){

  uint32_t ret;

  // display directories
  if (dir) offset = DisplayArray(false);
  else     offset = DisplayArray(true);

  if (! Valid_entry) {
    SERIALPORT.println(F("NO valid entry found"));
    return false;
  }
  if (dir)
    SERIALPORT.println(F("Which directory NUMBER do you want to select to remove? q = quit "));
  else
    SERIALPORT.println(F("Which file NUMBER do you want to select to remove? q = quit "));

  if (GetEntryNum(false) == -1) return false;

  if (sel > offset -1) {
    SERIALPORT.println(F("Invalid selection."));
    return false;
  }

  if (dir) {

    if (! DirContent[sel].isDir) {
      SERIALPORT.print(DirContent[sel].name);
      SERIALPORT.println(F(" Not a valid Directory."));
      return false;
    }

    // display any content in the directory
    else {

      // move in directory
      myLog.changeDirectory(DirContent[sel].name);

      sprintf(TempDir,"%s%s/", RootDir, DirContent[sel].name);
      sprintf(RootDir, "%s", TempDir);

      // list content
      list_files(true,false);

      // got back and thus one level up
      OneLevelUP(false);
    }
  }
  else {

    if (DirContent[sel].isDir) {
      SERIALPORT.print(DirContent[sel].name);
      SERIALPORT.println(F(" Not a valid file."));
      return false;
    }
  }

  // double check
  SERIALPORT.print(F("Are you sure you want to remove: "));
  SERIALPORT.println( DirContent[sel].name);
  SERIALPORT.println(F("( Y or N ). Must be a CAPITAL to make sure"));

  flush();

  while (! SERIALPORT.available()) yield();

  if (SERIALPORT.read() != 'Y') {
    SERIALPORT.println(F("Cancel remove (TIP : use capital Y if you want to remove)"));
    flush();
    return true;
  }

  if (dir) ret = myLog.removeDirectory((String) DirContent[sel].name);
  else     ret = myLog.removeFile((String) DirContent[sel].name);

  if (ret == 0) {
    SERIALPORT.print(F("Error during removing: "));
    SERIALPORT.println(DirContent[sel].name);
  }
  else {
    SERIALPORT.println(F("Removed "));
    SERIALPORT.print(ret);
    SERIALPORT.println(F(" Item(s)"));
  }

  return(list_files(true,false));
}

/*
 * read and display content of a file
 * @parameter Hex
 *  true  : display ASCII and HEX
 *  false : display ASCII
 *
 * @return
 *  true : if all went OK
 *  false: in case of error or cancel
 */
bool Read_file(bool hex)
{
   uint8_t j, i = 0;
   char bufhex[40];
   uint16_t fs;

  // display content of current directory
  offset = DisplayArray(true);

  if (! Valid_entry) {
    SERIALPORT.println(F("NO valid entry found."));
    return false;
  }

  SERIALPORT.println(F("Which file NUMBER do you want to select to read ? q = quit "));

  if (GetEntryNum(false) == -1) return false;

  if (sel > offset -1) {
    SERIALPORT.println(F("Invalid selection."));
    return false;
  }

  if (DirContent[sel].isDir) {
    SERIALPORT.println(F("Not a valid file."));
    return false;
  }

  // Get size of file
  int32_t sizeOfFile = myLog.size(DirContent[sel].name);

  if (sizeOfFile > -1)
  {
    if (sizeOfFile == 0) {
      SERIALPORT.print(DirContent[sel].name);
      SERIALPORT.println(F(" is empty.\n"));
      return true;
    }

    SERIALPORT.print(F("Content of "));
    SERIALPORT.print(DirContent[sel].name);
    SERIALPORT.print(F(" ("));
    SERIALPORT.print(sizeOfFile);
    SERIALPORT.println(F(" bytes)"));

    if (sizeOfFile > BUFLEN) {
      SERIALPORT.println(F("WARNING filesize is larger than buffer size."));
      SERIALPORT.print(F("For now this is a limitation and thus the maximum buffer size is used: "));
      SERIALPORT.println(BUFLEN);
      fs = BUFLEN;
      delay(1000);
    }
    else fs = sizeOfFile;

    myLog.read((uint8_t *)buf, fs, DirContent[sel].name); // Load with contents of myFile

    for (uint16_t y = 0 ; y < fs ; y++)
    {
      char ch = buf[y];

      if (hex) {

        if(ch < 0x10) SERIALPORT.print("0");

        SERIALPORT.print(ch, HEX);
        bufhex[i++] = ch;

        SERIALPORT.print(" ");

        if (i == 30) {
           for (j = i ; j < 40; j++) SERIALPORT.print(" ");

          for(j = 0 ; j < i ; j++) {
            if (bufhex[j] > 0x1f && bufhex[j] < 0xff) SERIALPORT.print(bufhex[j]);
            else SERIALPORT.print(".");
          }
          SERIALPORT.println("\r");
          i = 0;
        }
      }
      else {

        if (ch == '\r' || ch == '\n') {
          SERIALPORT.print(ch);
          i = 0;
        }
        else {
          if (ch > 0x1f) SERIALPORT.print(ch);
          else SERIALPORT.print("?");
          SERIALPORT.print(" ");

          if (i++ > 70) {
            SERIALPORT.println("\r");
            i = 0;
          }
        }
      }
    }

    // do we need to still display last line ASCII info
    if (hex && i != 0) {

      // complete HEX entry
      for(j = i ; j < 30 ; j++)  SERIALPORT.print("   ");  // 3 spaces each entry

      // include space
      for (j ; j < 40; j++) SERIALPORT.print(" ");

      // display rest for line
      for( j = 0 ; j < i ; j++){
        if (bufhex[j] > 0x1f  && bufhex[j] < 0xff) SERIALPORT.print(bufhex[j]);
        else SERIALPORT.print(".");
      }

      SERIALPORT.println("\r");

    }
  }
  else
  {
    SERIALPORT.println(F("Size error: File not found"));
    return false;
  }

  SERIALPORT.println(F("Done!"));
  return true;
}

/*
 * Add / append content to a file
 * if the file does not exist it will be created
 *
 * @return
 *  true : if all went OK
 *  false: in case of error or cancel
 */
bool Append_file()
{
  flush();

  SERIALPORT.println(F("Do you want to append content to an EXISTING file ? (Y or N)"));

  // wait for input
  while (! SERIALPORT.available()) yield();
  char c = SERIALPORT.read();
  flush();

  if (c == 'Y' || c == 'y') {

    // display files
    offset = DisplayArray(true);

    if (! Valid_entry) {
      SERIALPORT.println(F("NO valid file entry found."));
      return false;
    }

    SERIALPORT.println(F("Which file NUMBER do you want to select to append ? q = quit "));

    if (GetEntryNum(false) == -1) return false;

    if (sel > offset -1) {
      SERIALPORT.println("Invalid selection.");
      return false;
    }

    if (DirContent[sel].isDir) {
      SERIALPORT.print(DirContent[sel].name);
      SERIALPORT.println(F(" Not a valid file."));
      return false;
    }

    // copy name
    sprintf(fname, "%s", DirContent[sel].name);
  }

  else { // ask for the new filename

    SERIALPORT.print(F("Enter the name of the file to create. Only ENTER is abort."));

    uint8_t l = GetFileName();

    if (l == 0) return true;

    if (l > 12) {
      SERIALPORT.println(F("Filename to long. Max 12 characters - 8.3"));
      return false;
    }

    // check it exists
    long sizeOfFile = myLog.size(fname);

    if (sizeOfFile > -1) {
      SERIALPORT.println(F("Already exists"));
      return false;
    }
  }

  // get content to add
  SERIALPORT.print(F("Type content to add in file. Only enter is abort. : "));

  flush();
  offset = 0;

  // wait for input
  while(!SERIALPORT.available()) yield();
  c = SERIALPORT.read();

  while(c != '\r' && c != '\n' && offset < BUFLEN -1) {
    buf[offset++] = c;
    SERIALPORT.print(c);
    while (! SERIALPORT.available()) yield();
    c = SERIALPORT.read();
  }

  SERIALPORT.println();
  buf[offset] = 0x0;

  // if only enter
  if (offset == 0) return true;

  // do an append or create
  myLog.append(fname);

  // do write
  myLog.println(buf);

  // make sure it is written
  myLog.syncFile();

  return(true);
}

/*
 * Move one level up in directory structure
 * @parameter show : display content of directory after one level up
 *
 * @return
 *  true : if all went OK
 *  false: in case of error or cancel
 */
bool OneLevelUP(bool show)
{
  uint16_t i, j, s;

  // look for the last entry
  for (i = 0, j = 0; i < strlen(RootDir) ; i++) {
    if (RootDir[i] == '/' ) {
      s = j;  // second last
      j = i;  // last
    }
  }

  // returning to root
  if (j == 0) {
      if (show)  return(list_files(true,true));
      else       return(list_files(false,true));
  }

  // still level(s) directory down
  RootDir[s+1] = 0x0;

  // go to root
  ROOT;

  // change directory down to the right level
  // start at pos 1 to skip leading /
  for (i = 1, j = 0; i < strlen(RootDir); i++) {

    // end of a directory name
    if (RootDir[i] == '/' ){

      // change directory
      fname[j] = 0x0;
      myLog.changeDirectory(fname);
      j = 0;
    }
    else {
      // append to directory name
      fname[j++] = RootDir[i];
    }
  }

  if (show)  return(list_files(true,false));
  else       return(list_files(false,false));
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

    // end of array entries
    if (! strlen(DirContent[offset].name)) break;

    // check directory entry is from a file (else skip)
    if (CheckFile) {
      if (DirContent[offset].isDir) continue;
    }
    // check directory entry is from a directory (else skip)
    else {
      if (! DirContent[offset].isDir) continue;
    }

    // valid entry found
    Valid_entry = true;

    SERIALPORT.print(offset);
    SERIALPORT.print("\t");

    // determine number of spaces
    uint8_t n = 20 - strlen(DirContent[offset].name);

    SERIALPORT.print(DirContent[offset].name);

    // add enough spaces to align output
    while(n--) SERIALPORT.print(" ");

    if(!DirContent[offset].isDir){
      // Get size of file
      int32_t sizeOfFile = myLog.size(DirContent[offset].name);
      SERIALPORT.println(sizeOfFile);
    }
    else
      SERIALPORT.println("--");
  }

  return(offset);
}

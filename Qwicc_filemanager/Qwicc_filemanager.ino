/*
  QWICC OpenLog filemanager
  
  By: Paulvha@hotmail.com
  Date: March, 2022
  
 *  ================================ Disclaimer ======================================
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  ===================================================================================

  This has been tested on a Sparkfun Qwicc Openlog module (DEV-15164) connected to an Artemis ATP.

//=========================== Versioning ===============================

  v1.0                                / March 2022
  initial version
*/

#include <Wire.h>
#include "SparkFun_Qwiic_OpenLog_Arduino_Library.h"

//======================== user defines ==================================

#define MAXFILENAME 14                       // maximum length file or directory name (size is hardcoded in Openlog source)   
#define MAX_DIR_ENTRY 200                    // maximum entries in a current directory
#define EXTRA_BUF 10000                      // Add extra buffer size                
#define BUFLEN  (MAXFILENAME * MAX_DIR_ENTRY) + EXTRA_BUF  // max length to read directory content

// make sure the bufferlength fits 
#if BUFLEN > 65535
#error : BUFLEN larger than uint16_t size 65535
#endif
//======================== Global Variables ==============================

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
char RootDir[MAX_DIR_ENTRY * 5];
char TempDir[MAX_DIR_ENTRY * 5];

bool Valid_entry = false;                   // was valid entry was shown during DisplayArray
//============================ Start of program ===============================

void setup()
{
  Serial.begin(115200);
  while (!Serial); 
  Serial.println(F("QWICC OpenLog filemanager. (V1.0)\nPress Enter to continue."));
  GetFileName();
  
  Wire.begin();
  Wire.setClock(400000);
  myLog.begin();                            // Open connection to QWICC OpenLog

  // read root directory into buffer
  list_files(false, true);
}

void loop()
{
  Serial.print("\nCurrent directory :");
  Serial.println(RootDir);
  Serial.println(F("1  = list the files in current directory"));
  Serial.println(F("2  = Change directory"));
  Serial.println(F("3  = Create new directory"));
  Serial.println(F("4  = Create new file"));
  Serial.println(F("5  = Back to root directory"));
  Serial.println(F("6  = Remove file"));
  Serial.println(F("7  = Remove directory"));
  Serial.println(F("8  = One level up to parent directory"));
  Serial.println(F("9  = Read file. Display in ASCII only"));
  Serial.println(F("10 = Read file. Display in HEX and ASCII"));
  Serial.println(F("11 = Add content to file"));
  Serial.println(F("15 = Close SD"));
  Serial.print(F("\nYour input (? = help)") );

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
      Serial.println(F("Bye. Have a great day.\nPress 'reset' to restart."));
      while(1);
      break;
      
    default:
      Serial.print(F("Invalid input 0x"));
      Serial.println(sel, HEX);
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
    c = Serial.read();

    if (c == '?') {
      DisplayOptions(ext);
      ABR_input = c = '0';
      sel = 0;
    }
    else
      Serial.print(c);
  }
  
idone:
  Serial.println();
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
  Serial.println("\nEnter a number from the menu selection");
  if (! ext) return;
  Serial.println("\t\tOR\nls\tlist directory");
  Serial.println("cd\tChange directory");
  Serial.println("ro\tChange to root directory");
  Serial.println("rf\tRemove file");
  Serial.println("rd\tRemove directory");
  Serial.println("up\tChange to parent directory of current");
  Serial.println("ad\tDisplay file content in ASCII");
  Serial.println("hd\tDisplay file content in HEX and ASCII");
  Serial.println("ap\tAppend test to existing or new file.");
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
 *  Note 1 :
 *  I have tried to use searchDirectory() and then getNextDirectoryItem() as in example7. But that 
 *  construction only works ONCE. Performing the same a second time will not provide anything.
 *  A special function has been written and included in this sketch OpenLog_Dir_list()
 */
bool list_files(bool show,bool root)
{  
  offset = 0;                     // where to store next directory entry
  bool NoEntryDetected = true;    // detect empty directory
  bool ShowOnce = true;           // show header once
  char DirEntry[15];              // store each directory entry
  uint8_t n = 0;                  // length of characters in entry
  
  if (root) {
    ROOT;                         // rewind to root
    sprintf(RootDir,"%s","/");  
  }

  String myFile = "*";            // get all entries
  
  OpenLog_Dir_list((uint8_t *) buf, BUFLEN, myFile);  // see note1 above
  
  // parse the received content  
  for (uint16_t i = 0 ; i < BUFLEN ; i++) {

    // if not fillers for entry name
    if (buf[i] != 0xFF) {
     
      DirEntry[n++] = buf[i];

      // end of full entry name
      if(buf[i] == 0x0) {

        // only 0x0 in buffer
        if (n == 1) {
          n = 0;
          continue;
        }

        // at least ONE directory entry has been detected
        NoEntryDetected = false;
        
        // remove '/' which was added to indicate directory
        if (DirEntry[n-2] == '/'){
          DirContent[offset].isDir = true;
          DirEntry[n - 2] = 0x0;
          n--;
        }
        else 
          DirContent[offset].isDir = false;
        
        // add entry in array
        sprintf(DirContent[offset].name,"%s",DirEntry);

        if (show) {
          
          if (ShowOnce) {
            Serial.print("\nContent for ");
            Serial.println(RootDir);
            ShowOnce = false;  
          }
          
          Serial.print(DirEntry);
          
          // add enough spaces to align output
          for ( ; n <= 20; n++) Serial.print(" ");
          
          if(DirContent[offset].isDir) Serial.print("DIR \t");
          else Serial.print("FILE\t");   
                  
          // Get size of file
          int32_t sizeOfFile = myLog.size(DirEntry);  
          Serial.println(sizeOfFile);
        }

        // reset character counter in directory entry
        n = 0;   

        offset++;
    
        if (offset == MAX_DIR_ENTRY) {
          Serial.println("WARNING : can not add more entrys in array\n");
          offset--;
        }
      }
    }
  }
  
  // terminate last entry
  DirContent[offset].name[0] = 0x0;

  if(show & NoEntryDetected) {
    Serial.println("Empty directory\r");
  }
  
  return true;
}

/*
 * change to a sub-directory
 * @parameter ind : offset in array is already known
 */
bool change_dir(uint8_t ind)
{
  if (ind == 0) {
    
    // display directories
    offset = DisplayArray(false);
    
    if (! Valid_entry) {
      Serial.println(F("NO valid directory found."));
      return false;
    }
    
    Serial.println(F("Which directory NUMBER do you want to select ? q = quit "));
    
    if (GetEntryNum(false) == -1) return false;
  
    if (sel > offset -1) {
      Serial.println("Invalid selection.");
      return false;
    }
  }
  else
    sel = ind; 
 
  if (! DirContent[sel].isDir) {
    Serial.println("Not a valid directory.");
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
 */
bool create_entry(bool dir)
{
  offset = 0 ;

  if (dir)
    Serial.print(F("Enter the name of the directory to create. Only enter is abort. "));
  else
    Serial.print(F("Enter the name of the file to create. Only enter is abort. "));

  if (GetFileName() == 0) return true;
  
  // check it exists 
  long sizeOfFile = myLog.size(fname);

  if (sizeOfFile > -1) {
    Serial.println(F("Already exists"));
    return false;
  } 

  if (dir) {
    if ( ! myLog.makeDirectory((String) fname)) {
      Serial.println(F("Error during creating directory."));
    }
    else {
      Serial.println(F("Directory has been created."));
    }
  }
  else  // file
  {
    if ( ! myLog.create((String) fname)) {
      Serial.println(F("Failed to create file."));
    }
    else {
      Serial.println(F("File has been created."));
    }
  }
  
  return(list_files(true,false));
}

/*
 * remove file or directory
 * @param dir : true for directory
 */
bool remove_entry(bool dir){
  
  uint32_t ret;

  // display directories
  if (dir) offset = DisplayArray(false);
  else     offset = DisplayArray(true);

  if (! Valid_entry) {
    Serial.println(F("NO valid entry found"));
    return false;
  }
  if (dir)
    Serial.println(F("Which directory NUMBER do you want to select to remove? q = quit "));
  else
    Serial.println(F("Which file NUMBER do you want to select to remove? q = quit "));
  
  if (GetEntryNum(false) == -1) return false;
    
  if (sel > offset -1) {
    Serial.println(F("Invalid selection."));
    return false;
  }

  if (dir) {

    if (! DirContent[sel].isDir) {
      Serial.print(DirContent[sel].name);
      Serial.println(F(" Not a valid Directory."));
      return false;
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
    Serial.println(F("Cancel remove (TIP : use capital Y if you want to remove)"));
    flush();
    return true;
  }
  
  if (dir) ret = myLog.removeDirectory((String) DirContent[sel].name);
  else     ret = myLog.removeFile((String) DirContent[sel].name);  

  if (ret == 0) {
    Serial.print(F("Error during removing: ")); 
    Serial.println(DirContent[sel].name);
  }
  else {
    Serial.println(F("Removed "));
    Serial.print(ret);
    Serial.println(F(" Item(s)"));
  }

  return(list_files(true,false));
}

/*
 * read and display content of a file
 * @parameter Hex
 *  true  : display ASCII and HEX
 *  false : display ASCII
 */
bool Read_file(bool hex)
{
   uint8_t j, i = 0;
   char bufhex[40];
   uint16_t fs;
   
  // display content of current directory
  offset = DisplayArray(true);
  
  if (! Valid_entry) {
    Serial.println(F("NO valid entry found."));
    return false;
  }
  
  Serial.println(F("Which file NUMBER do you want to select to read ? q = quit "));

  if (GetEntryNum(false) == -1) return false;   

  if (sel > offset -1) {
    Serial.println(F("Invalid selection."));
    return false;
  }
  
  if (DirContent[sel].isDir) {
    Serial.println(F("Not a valid file."));
    return false;
  }
  
  // Get size of file
  int32_t sizeOfFile = myLog.size(DirContent[sel].name);

  if (sizeOfFile > -1)
  {
    if (sizeOfFile == 0) {
      Serial.print(DirContent[sel].name);
      Serial.println(F(" is empty.\n"));
      return true;
    }

    Serial.print(F("Content of "));
    Serial.print(DirContent[sel].name);
    Serial.print(F(" ("));
    Serial.print(sizeOfFile);
    Serial.println(F(" bytes)"));
    
    if (sizeOfFile > BUFLEN) {
      Serial.println("WARNING filesize is larger than buffer size.");
      Serial.print("For now this is a limitation and thus the maximum buffer size is used: ");
      Serial.println(BUFLEN);
      fs = BUFLEN;
      delay(1000);
    }
    else fs = sizeOfFile;
     
    myLog.read((uint8_t *)buf, fs, DirContent[sel].name); // Load with contents of myFile

    for (uint16_t y = 0 ; y < fs ; y++)
    {
      char ch = buf[y];

      if (hex) {
        
        if(ch < 0x10) Serial.print("0");
        
        Serial.print(ch, HEX);
        bufhex[i++] = ch;

        Serial.print(" ");
        
        if (i == 30) {
           for (j = i ; j < 40; j++) Serial.print(" ");
          
          for(j = 0 ; j < i ; j++) {
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
      for(j = i ; j < 30 ; j++)  Serial.print("   ");  // 3 spaces each entry
      
      // include space
      for (j ; j < 40; j++) Serial.print(" ");
      
      // display rest for line
      for( j = 0 ; j < i ; j++){
        if (bufhex[j] > 0x1f) Serial.print(bufhex[j]);
        else Serial.print(".");
      }
      
      Serial.println("\r");

    }
  }
  else
  {
    Serial.println(F("Size error: File not found"));
    return false;
  }

  Serial.println(F("Done!"));
  return true;
}

/*
 * Add / append content to a file
 * if the file does not exist it will be created
 */
bool Append_file()
{
  flush();
  
  Serial.println(F("Do you want to append content to an EXISTING file ? (Y or N)"));

  // wait for input
  while (! Serial.available()) yield();
  char c = Serial.read();
  flush();
  
  if (c == 'Y' || c == 'y') {
    
    // display files
    offset = DisplayArray(true);
    
    if (! Valid_entry) {
      Serial.println(F("NO valid file entry found."));
      return false;
    } 
    
    Serial.println(F("Which file NUMBER do you want to select to append ? q = quit "));
  
    if (GetEntryNum(false) == -1) return false;   
  
    if (sel > offset -1) {
      Serial.println("Invalid selection.");
      return false;
    }
    
    if (DirContent[sel].isDir) {
      Serial.print(DirContent[sel].name);
      Serial.println(F(" Not a valid file."));
      return false;
    }
  
    // copy name
    sprintf(fname, "%s", DirContent[sel].name);
  }
  
  else { // ask for the new filename
  
    Serial.print(F("Enter the name of the file to create. Only ENTER is abort."));
    
    uint8_t l = GetFileName();
    
    if (l == 0) return true;

    if (l > 12) {
      Serial.println("Filename to long. Max 12 characters - 8.3");
      return false;
    }
    
    // check it exists 
    long sizeOfFile = myLog.size(fname);
  
    if (sizeOfFile > -1) {
      Serial.println(F("Already exists"));
      return false;
    } 
  }

  // get content to add
  Serial.print(F("Type content to add in file. Only enter is abort. : "));
  
  flush();
  offset = 0;
  
  // wait for input
  while(!Serial.available()) yield();
  c = Serial.read();

  while(c != '\r' && c != '\n' && offset < BUFLEN -1) {
    buf[offset++] = c;
    Serial.print(c);
    while (! Serial.available()) yield();
    c = Serial.read();
  }
  
  Serial.println();
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
      if ( DirContent[offset].isDir) {
        Valid_entry = true;
        Serial.println(DirContent[offset].name);
      }
      else Serial.println(F("\tFile. can not select"));
    }
  }
  
  return(offset);
}

//==================================================================================================
// Special function which should actually be a library function
//
// I have tried to use searchDirectory() and then getNextDirectoryItem() as in example7. But that 
// construction only works ONCE. Performing the same a second time will not provide anything.
// This special function has been written to take different approach.
//
// following assumptions:
//
// Standard Wire is used
// Standard address is used
// I2C Buffer size is taken from library
// Sendcommand is public function in library
// The complete directory list will fit within the buffersize
//
// Read the contents of a directory, up to the size of the buffer, into a given array, 
//==================================================================================================
#define LIST_COMMAND 0x0e
#define DEV_ADDRR    QOL_DEFAULT_ADDRESS
#define OWIRE        Wire

void OpenLog_Dir_list(uint8_t *userBuffer, uint16_t bufferSize, String fileName)
{
  uint16_t spotInBuffer = 0;
  uint16_t leftToRead = bufferSize; // Read up to the size of our buffer. We may go past EOF.
  myLog.sendCommand(LIST_COMMAND, fileName);
  
  // Upon completion Qwiic OpenLog will respond with the file contents. Master can request up to 32 bytes at a time.
  // Qwiic OpenLog will respond until it reaches the end of file then it will report zeros.

  while (leftToRead > 0)
  {
    uint8_t toGet = I2C_BUFFER_LENGTH; // Request up to a 32 byte block
    if (leftToRead < toGet)
      toGet = leftToRead; // Go smaller if that's all we have left

    OWIRE.requestFrom(DEV_ADDRR, toGet);
    while (OWIRE.available())
      userBuffer[spotInBuffer++] = OWIRE.read();

    leftToRead -= toGet;
  }
}

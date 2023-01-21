/*
 * This sketch is designed to be used on Apollo3 / Artemis boards from Sparkfun
 * It will display memory location in HEX and in ASCII (if printable character).
 * 
 * Not all memory locations are valid can cause a crash. Often the blue light on
 * the processor board will blink and the display will only show the start address.
 * 
 * In combination with the Apollo3 datasheet you can try to debug the memory
 * 
 * Output looks like :
  Offset: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F   
40008000: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
40008010: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
40008020: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
40008030: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................

  Version 1.0 / January 2023 /paulvha
*/

#define MAXINPUT   20 
char Input[MAXINPUT];

uint32_t StartLocation=0;
uint32_t Length = 0;
uint8_t  bufhex[16];

void setup() {
  
  Serial.begin(115200);
  while(!Serial) delay(100);

  Serial.println("\nMaking memory dump.\nPress Enter to continue.");
  GetInput();
}

void loop() {
  char *p; 
  uint8_t l, sl;
  static int cnt =0;
  while(1) {
    Serial.print("Enter start address in Hex. ");
    if (StartLocation > 0) Serial.print("Only <enter> to use previous location. ");
    Serial.print("0x"); 
    
    l = GetInput();
    
    if (l > 0)  {
      StartLocation = strtol(Input,&p,16);
    
      if (*p != 0x0 )  {
        Serial.printf("Invalid number: %s\n",Input);
        continue;
      }
      
      sl = l;
    }
     
    Serial.print("Enter length in Hex. ");
    if (Length > 0) Serial.print("Only <enter> to use previous length. ");
    Serial.print("0x"); 
    
    if (GetInput() > 0){
      Length = strtol(Input,&p,16); 
  
      if (*p != 0x0)  {
        Serial.printf("Invalid Length number: %s\n ",Input);
        continue;
      }
    }
  
    if(Length == 0){
      Serial.println("Length can NOT be zero");
      continue;
    }
    
    MakeDump(sl);
  }
}

/**
 * display the HExbuffer in ASCII
 */
void DispAscii(uint8_t j){
  Serial.print("\t");
  for (uint8_t x = 0 ; x < j; x++){
    if (bufhex[x] > 0x1f) Serial.print((char) bufhex[x]);
    else Serial.print(".");
  }
}

/**
 * perform the memory dump
 * @param l : #bytes in StartLocation
 */
void MakeDump(uint8_t l )
{
  uint8_t j;
  uint32_t i;
  
  Serial.print("Offset:\t\t00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F"); 
  
  for (i = 0, j = 0 ; i < Length; i++){
    // new line ?
    if (i%16 == 0){
      Serial.printf("\n0x%x\t",StartLocation+i);
      if (l < 7) Serial.print("\t");
    }
    
    // display HEX value
    uint8_t p= AM_REGVAL(StartLocation+i);
    if(p < 0x10) Serial.print("0");
    Serial.printf("%x ",p);
    bufhex[j++] = p;

    // display ascii ?
    if(j == 16) {      
      DispAscii(j);
      j=0;
    }
  }

  if(j > 0){
    for (uint8_t x = j * 3; x < 16 * 3; x++) Serial.print(" ");
    DispAscii(j);
  }

  Serial.println();
}

/**
 * empty serial buffer
 */
void flush(){
  delay(500);

  while (Serial.available()) {
    Serial.read();
    delay(500);
    yield();
  }
}

/**
 * read a input and store in global variable Input
 * @ return number of characters in Input
 */
uint8_t GetInput()
{
  uint8_t offset = 0;

  flush();

  // wait for input
  while(!Serial.available());
  char c = Serial.read();

  while(c != '\r' && c != '\n' && offset < MAXINPUT -1) {
    Input[offset++] = c;
    Serial.print(c);
    while (! Serial.available()) yield();
    c = Serial.read();
  }

  Serial.println();

  Input[offset] = 0x0;

  return(offset);
}

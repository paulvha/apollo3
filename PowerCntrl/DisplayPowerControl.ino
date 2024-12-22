/**
 * This file contains different routines to display power control registers
 * 
 * display_buck()
 * display_peripherals()
 * display_memory()
 * display_events()
 * 
 * version 1.0 / February 2023 / paulvha
 * 
 */
void display_buck() {

  Serial.print("\nSUPPLYSRC 0x");
  Serial.println(PWRCTRL->SUPPLYSRC, HEX);

  if (PWRCTRL->SUPPLYSRC_b.BLEBUCKEN == 1) Serial.println("\tBLE Buck is enabled");
  else Serial.println("\tBLE Buck is disabled");


  Serial.print("\nSUPPLYSTATUS 0x");
  Serial.println(PWRCTRL->SUPPLYSTATUS, HEX); 

  if (PWRCTRL->SUPPLYSTATUS_b.SIMOBUCKON) Serial.println("\tSIMO Buck is ON");
  else Serial.println("\tSIMO Buck is OFF");

  if (PWRCTRL->SUPPLYSTATUS_b.BLEBUCKON) Serial.println("\tBuck is supplying the BLE/Burst power domain");
  else Serial.println("\tLDO is supplying the BLE/Burst power domain");


  Serial.print("\nMISC 0x");
  Serial.println(PWRCTRL->MISC, HEX); 

  if (PWRCTRL->MISC_b.FORCEMEMVRLPTIMERS) Serial.println("\tforce Mem VR to LP mode in deep sleep even when hfrcbased ctimer or stimer is running");
  if (PWRCTRL->MISC_b.MEMVRLPBLE) Serial.println("\tlet Mem VR go to lp mode in deep sleep even when BLEL or BLEH is powered on given none of the other domains require it.  ");
}


void display_peripherals() {

  Serial.print("\nDEVPWREN 0x");
  Serial.println(PWRCTRL->DEVPWREN, HEX);

  if (PWRCTRL->DEVPWREN_b.PWRIOS == 1) Serial.println("\tIO slave is set on");
  if (PWRCTRL->DEVPWREN_b.PWRIOM0 == 1) Serial.println("\tIOM0 is set on");
  if (PWRCTRL->DEVPWREN_b.PWRIOM1 == 1) Serial.println("\tIOM1 is set on");
  if (PWRCTRL->DEVPWREN_b.PWRIOM2 == 1) Serial.println("\tIOM2 is set on");
  if (PWRCTRL->DEVPWREN_b.PWRIOM3 == 1) Serial.println("\tIOM3 is set on");
  if (PWRCTRL->DEVPWREN_b.PWRIOM4 == 1) Serial.println("\tIOM4 is set on");
  if (PWRCTRL->DEVPWREN_b.PWRIOM5 == 1) Serial.println("\tIOM5 is set on");
  if (PWRCTRL->DEVPWREN_b.PWRUART0 == 1) Serial.println("\tUART0 is set on");
  if (PWRCTRL->DEVPWREN_b.PWRUART1 == 1) Serial.println("\tUART1 is set on");
  if (PWRCTRL->DEVPWREN_b.PWRADC == 1) Serial.println("\tADC is set on");
  if (PWRCTRL->DEVPWREN_b.PWRSCARD == 1) Serial.println("\tScard is set on");
  if (PWRCTRL->DEVPWREN_b.PWRMSPI == 1) Serial.println("\tSPI is set on");
  if (PWRCTRL->DEVPWREN_b.PWRPDM == 1) Serial.println("\tPDM is set on");
  if (PWRCTRL->DEVPWREN_b.PWRBLEL == 1) Serial.println("\tBLE is set on");

  
  Serial.print("\nDEVPWRSTATUS 0x");
  Serial.println(PWRCTRL->DEVPWRSTATUS, HEX);

  if (PWRCTRL->DEVPWRSTATUS_b.MCUL == 1) Serial.println("\tMCUL (DMA and Fabrics) is set on");
  if (PWRCTRL->DEVPWRSTATUS_b.MCUH == 1) Serial.println("\tMCUH (ARM core) is set on");
  if (PWRCTRL->DEVPWRSTATUS_b.HCPA == 1) Serial.println("\tHCPA (IOS,UART0,UART1) is set on");
  if (PWRCTRL->DEVPWRSTATUS_b.HCPB == 1) Serial.println("\tHCPB (IOM0,IOM1,IOM2) is set on");
  if (PWRCTRL->DEVPWRSTATUS_b.HCPC == 1) Serial.println("\tHCPC (IOM3,IOM4,IOM5) is set on");
  if (PWRCTRL->DEVPWRSTATUS_b.PWRADC == 1) Serial.println("\tPWRADC is set on");
  if (PWRCTRL->DEVPWRSTATUS_b.PWRMSPI == 1) Serial.println("\tPWRMSPIis set on");
  if (PWRCTRL->DEVPWRSTATUS_b.PWRPDM == 1) Serial.println("\tPWRPDM is set on");
  if (PWRCTRL->DEVPWRSTATUS_b.BLEL == 1) Serial.println("\tBLEL (BLE controller) is set on");
  if (PWRCTRL->DEVPWRSTATUS_b.BLEH == 1) Serial.println("\tBLEH (BLE power) is set on");
}

void display_memory(){

  Serial.print("\nMEMPWDINSLEEP 0x");
  Serial.println(PWRCTRL->MEMPWDINSLEEP, HEX);

  switch(PWRCTRL->MEMPWDINSLEEP_b.DTCMPWDSLP) {
    case 0: 
      Serial.println("\tAll DTCM retained");
      break;
    case 1: 
      Serial.println("\tGroup0_DTCM0 powered down in deep sleep (0KB-8KB)");
      break;
    case 2: 
      Serial.println("\tGroup0_DTCM1 powered down in deep sleep (8KB-32KB)");
      break;
    case 3: 
      Serial.println("\tBoth DTCMs in group0 are powered down in deep sleep (0KB-32KB)");
      break;
    case 4: 
      Serial.println("\tGroup1 DTCM powered down in deep sleep (32KB-64KB)");
      break;
    case 6: 
      Serial.println("\tGroup1 and Group0_DTCM1 are powered down in deep sleep (8KB-64KB)");
      break;
    case 7: 
      Serial.println("\tAll DTCMs powered down in deep sleep (0KB-64KB)");
      break;
    default:
      Serial.print("\tUnknown value 0x");
      Serial.println(PWRCTRL->MEMPWDINSLEEP_b.DTCMPWDSLP);
      break;
  }

 switch(PWRCTRL->MEMPWDINSLEEP_b.SRAMPWDSLP) {
    case 0: 
      Serial.println("\tAll SRAM banks retained");
      break;
    case 1: 
      Serial.println("\tSRAM GROUP0 powered down (64KB-96KB)");
      break;
    case 0x2: 
      Serial.println("\tSRAM GROUP0 powered down (96KB-128KB)");
      break;
    case 0x4: 
      Serial.println("\tSRAM GROUP0 powered down (128KB-160KB)");
      break;
    case 0x8: 
      Serial.println("\tSRAM GROUP0 powered down (160KB-192KB)");
      break;
    case 0x10: 
      Serial.println("\tSRAM GROUP0 powered down (192KB-224KB)");
      break;
    case 0x20: 
      Serial.println("\tSRAM GROUP0 powered down (224KB-256KB)");
      break;
    case 0x40: 
      Serial.println("\tSRAM GROUP0 powered down (256KB-288KB)");
      break;
    case 0x80: 
      Serial.println("\tSRAM GROUP0 powered down (288KB-320KB)");
      break;
    case 0x100: 
      Serial.println("\tSRAM GROUP0 powered down (320KB-352KB)");
      break;
    case 0x200: 
      Serial.println("\tSRAM GROUP0 powered down (352KB-384KB)");
      break;
    case 0x3: 
      Serial.println("\tPower-down lower 64k SRAM (64KB-128KB)");
      break;
    case 0xF: 
      Serial.println("\tPower-down lower 128k SRAM (64KB-192KB)");
      break;
    case 0x3fe: 
      Serial.println("\tAll SRAM banks but lower 32k powered down (96KB-384KB)");
      break;
    case 0x3fc: 
      Serial.println("\tAll SRAM banks but lower 64k powered down (128KB-384KB");
      break;
    case 0x3f0: 
      Serial.println("\tAll SRAM banks but lower 128k powered down (196KB-384KB)");
      break;
    case 0x3ff: 
      Serial.println("\tAll SRAM banks powered down in deep sleep");
      break;
    default:
      Serial.print("\tUnknown value 0x");
      Serial.println(PWRCTRL->MEMPWDINSLEEP_b.SRAMPWDSLP);
      break;
  }

  if (PWRCTRL->MEMPWDINSLEEP_b.FLASH0PWDSLP == 1) Serial.println("\tFlash0 is powered down during deepsleep");
  else Serial.println("\tFlash0 is kept powered on during deepsleep");
  
  if (PWRCTRL->MEMPWDINSLEEP_b.FLASH1PWDSLP == 1) Serial.println("\tFlash1 is powered down during deepsleep");
  else Serial.println("\tFlash1 is kept powered on during deepsleep");
  
  if (PWRCTRL->MEMPWDINSLEEP_b.CACHEPWDSLP == 1) Serial.println("\tcache is powered down in deepsleep");
  else Serial.println("\tcache is kept powered on during deepsleep");
  
  Serial.print("\nMEMPWREN 0x");
  Serial.println(PWRCTRL->MEMPWREN, HEX);

    switch(PWRCTRL->MEMPWREN_b.DTCM) {
    case 0: 
      Serial.println("\tDo not enable power to any DTCMs");
      break;
    case 1: 
      Serial.println("\tPower enable only GROUP0_DTCM0");
      break;
    case 2: 
      Serial.println("\tPower enable only GROUP0_DTCM1");
      break;
    case 3: 
      Serial.println("\tPower enable only DTCMs in GROUP0");
      break;
    case 4: 
      Serial.println("\tPower enable only DTCMs in GROUP1");
      break;
    case 6: 
      Serial.println("\tGroup1 and Group0_DTCM1 are powered down in deep sleep (8KB-64KB)");
      break;
    case 7: 
      Serial.println("\tPower enable all DTCMs");
      break;
    default:
      Serial.print("\tUnknown value 0x");
      Serial.println(PWRCTRL->MEMPWREN_b.DTCM);
      break;
  }

 switch(PWRCTRL->MEMPWREN_b.SRAM) {
    case 0: 
      Serial.println("\tDo not power enable any of the SRAM banks");
      break;
    case 1: 
      Serial.println("\tPower enable only SRAM group0 (0KB-32KB)");
      break;
    case 0x2: 
      Serial.println("\tPower enable only SRAM group1 (32KB-64KB)");
      break;
    case 0x4: 
      Serial.println("\tPower enable only SRAM group2(64KB-96KB)");
      break;
    case 0x8: 
      Serial.println("\tPower enable only SRAM group3 (96KB-128KB)");
      break;
    case 0x10: 
      Serial.println("\tPower enable only SRAM group4 (128KB-160KB)");
      break;
    case 0x20: 
      Serial.println("\tPower enable only SRAM group5 (160KB-192KB)");
      break;
    case 0x40: 
      Serial.println("\tPower enable only SRAM group6 (192KB-244KB)");
      break;
    case 0x80: 
      Serial.println("\tPower enable only SRAM group7 (244KB-256KB)");
      break;
    case 0x100: 
      Serial.println("\tPower enable only SRAM group8 (256KB-288KB)");
      break;
    case 0x200: 
      Serial.println("\tPower enable only SRAM group9 (288KB-320KB)");
      break;
    case 0x3: 
      Serial.println("\tPower enable only lower 64k SRAM");
      break;
    case 0xF: 
      Serial.println("\tPower enable only lower 128K SRAM");
      break;
    case 0xFF: 
      Serial.println("\tPower enable only lower 256K SRAM");
      break;
    case 0x3ff: 
      Serial.println("\tPower enable All SRAM banks");
      break;
    default:
      Serial.print("\tUnknown value 0x");
      Serial.println(PWRCTRL->MEMPWREN_b.SRAM);
      break;
  }

  if (PWRCTRL->MEMPWREN_b.FLASH0 == 1) Serial.println("\tPower enable Flash0");
  if (PWRCTRL->MEMPWREN_b.FLASH1 == 1) Serial.println("\tPower enable Flash1");
  if (PWRCTRL->MEMPWREN_b.CACHEB0 == 1) Serial.println("\tPower enable Cache Bank 0");
  if (PWRCTRL->MEMPWREN_b.CACHEB2 == 1) Serial.println("\tPower enable Cache Bank 2");

  Serial.print("\nMEMPWRSTATUS 0x");
  Serial.println(PWRCTRL->MEMPWRSTATUS, HEX);

  if (PWRCTRL->MEMPWRSTATUS_b.DTCM00 == 1) Serial.println("\tpower is supplied to DTCM GROUP0_0");
  if (PWRCTRL->MEMPWRSTATUS_b.DTCM01 == 1) Serial.println("\tpower is supplied to DTCM GROUP0_1");
  if (PWRCTRL->MEMPWRSTATUS_b.DTCM1 == 1) Serial.println("\tpower is supplied to DTCM GROUP1");
  if (PWRCTRL->MEMPWRSTATUS_b.SRAM0 == 1) Serial.println("\tpower is supplied to SRAM GROUP0");
  if (PWRCTRL->MEMPWRSTATUS_b.SRAM1 == 1) Serial.println("\tpower is supplied to SRAM GROUP1");
  if (PWRCTRL->MEMPWRSTATUS_b.SRAM2 == 1) Serial.println("\tpower is supplied to SRAM GROUP2");
  if (PWRCTRL->MEMPWRSTATUS_b.SRAM3 == 1) Serial.println("\tpower is supplied to SRAM GROUP3");
  if (PWRCTRL->MEMPWRSTATUS_b.SRAM4 == 1) Serial.println("\tpower is supplied to SRAM GROUP4");
  if (PWRCTRL->MEMPWRSTATUS_b.SRAM5 == 1) Serial.println("\tpower is supplied to SRAM GROUP5");
  if (PWRCTRL->MEMPWRSTATUS_b.SRAM6 == 1) Serial.println("\tpower is supplied to SRAM GROUP6");
  if (PWRCTRL->MEMPWRSTATUS_b.SRAM7 == 1) Serial.println("\tpower is supplied to SRAM GROUP7");
  if (PWRCTRL->MEMPWRSTATUS_b.SRAM8 == 1) Serial.println("\tpower is supplied to SRAM GROUP8");
  if (PWRCTRL->MEMPWRSTATUS_b.SRAM9 == 1) Serial.println("\tpower is supplied to SRAM GROUP9");
  if (PWRCTRL->MEMPWRSTATUS_b.FLASH0 == 1) Serial.println("\tpower is supplied to FLASH0");
  if (PWRCTRL->MEMPWRSTATUS_b.FLASH1 == 1) Serial.println("\tpower is supplied to FLASH1");
  if (PWRCTRL->MEMPWRSTATUS_b.CACHEB0 == 1) Serial.println("\tpower is supplied to Cache Bank 0");
  if (PWRCTRL->MEMPWRSTATUS_b.CACHEB2 == 1) Serial.println("\tpower is supplied to Cache Bank 2");
}

void display_events() {
  Serial.print("\nDEVPWREVENTEN 0x");
  Serial.println(PWRCTRL->DEVPWREVENTEN, HEX);

  if (PWRCTRL->DEVPWREVENTEN == 0) Serial.println("\tNO peripheral power-on status event enabled");
  else {
    if (PWRCTRL->DEVPWREVENTEN_b.MCULEVEN == 1) Serial.println("\tEnable MCUL power-on status event");
    if (PWRCTRL->DEVPWREVENTEN_b.MCUHEVEN == 1) Serial.println("\tEnable MCUH power-on status event");
    if (PWRCTRL->DEVPWREVENTEN_b.HCPAEVEN == 1) Serial.println("\tEnable HCPA power-on status event");
    if (PWRCTRL->DEVPWREVENTEN_b.HCPBEVEN == 1) Serial.println("\tEnable HCPB power-on status event");
    if (PWRCTRL->DEVPWREVENTEN_b.HCPCEVEN == 1) Serial.println("\tEnable HCPC power-on status event");
    if (PWRCTRL->DEVPWREVENTEN_b.ADCEVEN == 1) Serial.println("\tEnable ADC power-on status event");
    if (PWRCTRL->DEVPWREVENTEN_b.MSPIEVEN == 1) Serial.println("\tEnable MSPI power-on status event");
    if (PWRCTRL->DEVPWREVENTEN_b.PDMEVEN == 1) Serial.println("\tEnable PDM power-on status event");
    if (PWRCTRL->DEVPWREVENTEN_b.BLELEVEN == 1) Serial.println("\tEnable BLE power-on status event");
    if (PWRCTRL->DEVPWREVENTEN_b.BLEFEATUREEVEN == 1) Serial.println("\tEnable BLEFEATURE power-on status event");
    if (PWRCTRL->DEVPWREVENTEN_b.BURSTFEATUREEVEN == 1) Serial.println("\tEnable BURSTFEATURE power-on status event");
    if (PWRCTRL->DEVPWREVENTEN_b.BURSTEVEN == 1) Serial.println("\tEnable BURST status event");
  }
  
  Serial.print("\nMEMPWREVENTEN 0x");
  Serial.println(PWRCTRL->MEMPWREVENTEN, HEX);

  if (PWRCTRL->MEMPWREVENTEN == 0) {
    Serial.println("\tNO memory power-on status event enabled");
    return;
  }

  switch(PWRCTRL->MEMPWREVENTEN_b.DTCMEN) {
    case 0: 
      Serial.println("\tDisabled DTCM power-on status event");
      break;
    case 1: 
      Serial.println("\tEnable GROUP0_DTCM0 power on status event");
      break;
    case 2: 
      Serial.println("\tEnable GROUP0_DTCM1 power on status event");
      break;
    case 3: 
      Serial.println("\tEnable DTCMs in group0 power on status event");
      break;
    case 4: 
      Serial.println("\tEnable DTCMs in group1 power on status event");
      break;
    case 7: 
      Serial.println("\tEnable all DTCMs power on status event");
      break;
    default:
      Serial.print("\tUnknown value 0x");
      Serial.println(PWRCTRL->MEMPWREVENTEN_b.DTCMEN);
      break;
  }

 switch(PWRCTRL->MEMPWREVENTEN_b.SRAMEN) {
    case 0: 
      Serial.println("\tDisabled SRAM power-on status event");
      break;
    case 1: 
      Serial.println("\tEnable SRAM group0 (0KB-32KB) power on status event");
      break;
    case 0x2: 
      Serial.println("\tEnable SRAM group1 (32KB-64KB) power on status event");
      break;
    case 0x4: 
      Serial.println("\tEnable SRAM group2 (64KB-96KB) power on status event");
      break;
    case 0x8: 
      Serial.println("\tEnable SRAM group3 (96KB-128KB) power on status event");
      break;
    case 0x10: 
      Serial.println("\tEnable SRAM group4 (128KB-160KB) power on status event");
      break;
    case 0x20: 
      Serial.println("\tEnable SRAM group5 (160KB-192KB) power on status event");
      break;
    case 0x40: 
      Serial.println("\tEnable SRAM group6 (192KB-224KB) power on status event");
      break;
    case 0x80: 
      Serial.println("\tEnable SRAM group7 (224KB-256KB) power on status event");
      break;
    case 0x100: 
      Serial.println("\tEnable SRAM group8 (256KB-288KB) power on status event");
      break;
    case 0x200: 
      Serial.println("\tEnable SRAM group9 (288KB-320KB) power on status event");
      break;
    default:
      Serial.print("\tUnknown value 0x");
      Serial.println(PWRCTRL->MEMPWREVENTEN_b.SRAMEN);
      break;
  }

  if (PWRCTRL->MEMPWREVENTEN_b.FLASH0EN == 1) Serial.println("\tEnable FLASH0 status event");
  if (PWRCTRL->MEMPWREVENTEN_b.FLASH1EN == 1) Serial.println("\tEnable FLASH1 status event");
  if (PWRCTRL->MEMPWREVENTEN_b.CACHEB0EN == 1) Serial.println("\tEnable CACHE BANK 0 status event");
  if (PWRCTRL->MEMPWREVENTEN_b.CACHEB2EN == 1) Serial.println("\tEnable CACHE BANK 2 status event");
  
}

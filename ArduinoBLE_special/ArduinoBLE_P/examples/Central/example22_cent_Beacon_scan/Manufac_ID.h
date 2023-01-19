/**
 * Different lookup tables 
 * 
 * Version 1.0 / January 2023 / Paulvha
 */
struct LookUp {
  uint16_t id;
  char     name[35];
};

// looup for manufacturer
LookUp m[] {
  {0x004C, "Apple"},
  {0x0003, "IBM"},
  {0x0006, "Microsoft"},
  {0x0013, "Atmel"},
  {0x002B, "Tenovis"},
  {0x0030, "STM"},      //ST Microelectronics
  {0x003F, "SIG"},      //Bluetooth SIG, Inc.
  {0x0056, "Sony"},
  {0x0059, "Nordic"},   //Nordic Semiconductor ASA 
  {0x0065, "HP"},       //Hewlett-Packard Company
  {0x009E, "Bose"},     //Bose Corporation
  {0x00B8, "Qualcomm"}, //Qualcomm Innovation Center, Inc. (QuIC) (seems to have a number more)
  {0x00CD, "Microchip"},//Microchip Technology Inc.
  {0x00D8, "Qualcomm"}, //Qualcomm Connected Experiences, Inc.
  {0x00E0, "Google"},
  {0x0118, "Radius"},   //Radius Networks, Inc.
  {0x0075, "Samsung"},  //Samsung Electronics Co. Ltd.
  {0x0822, "Adafruit"}, //adafruit industries
  {0x09A3, "Arduino"},  //ARDUINO SA
  {0x09AC, "Ambiq"}
};
uint8_t NumMan = sizeof(m) / sizeof(struct LookUp);


// extra flags
LookUp ff[]{
  {0x0001, "Flags"},
  {0x0002, "IncompleteAdvertisedService16"},
  {0x0003, "CompleteAdvertisedService16"},
  {0x0006, "IncompleteAdvertisedService128"},
  {0x0007, "CompleteAdvertisedService128"},
  {0x0008, "ShortLocalName"},
  {0x0009, "CompleteLocalName"},
  {0x0016, "ServiceData"},
  {0x00FF, "ManufacturerData"}
};
uint8_t Numff = sizeof(ff) / sizeof(struct LookUp);

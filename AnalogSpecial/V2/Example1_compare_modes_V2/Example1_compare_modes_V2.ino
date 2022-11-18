/*
 * This sketch demonstrates how analogReadTurbo() is compared to analogReadFast(),analogReadSingleFast() and analogRead() for
 * reading multiple ADC pins and a single pin.
 *
 * Version : 1.0
 * Date    : November 2022
 * author  : paulvha
 *
 * This example is only tested on V2.2.1 of the Sparkfun Apollo3 Library. It requires the special version of the CommonAnalog.cpp and common.h
 *
 * During a standard analogRead() implementation only ADC SLOT-0 is used. With each analogRead() there are large number of checks, followed
 * by a setup / re-init of SLOT-0 everytime as only this SLOT is used. All these checks are valid and necessary to prevent unexpected behaviour.
 *
 * In analogReadTurbo() & analogReadFast() & analogReadSingleFast() many of the checks are removed and expects the usage is only
 * to read the ADC value on one of the 10 ADC pins. Not all pins are exposed external on all Apollo3 boards. See the board pinout information on Sparkfun.com.
 *
 * analogReadTurbo() and analogReadFast() are mend to be used to read multiple ADC pins in a fast manner.
 * An Apollo3 has 8 ADC slots that can be used. In analogReadTurbo() and analogReadFast() setup each pin will get its own ADC-Slot assigned and enabled.
 * With each read-cyle all the enabled slots are read in one go. Although maximum 5 ADC-pins are possible (set by default in the adjusted driver), this example is using 3 pins
 *
 * analogReadSingleFast() is a modified analogRead() optimized to perform a fast reading of a SINGLE ADCpin. It can achieve a very high
 * frequency, but only on a single ADCPin.
 *
 * No matter analogReadTurbo(), analogReadFast(),analogReadSingleFast() or analogRead(), make sure to initialize with a dummy read in setup().
 * A first call will also initialize the Apollo3 module + ADC-slot.
 * That takes about 700us. Each next call with a new pin, will take ~80us to setup the slot.
 *
 *----------------------------------------------------------------------------------------------------------------------
 * analogReadTurbo()
 *
 * Setting Read_mode as 1 : With analogReadTurbo() the results of all enabled pins / slots read are returned in a structure to be processed / displayed later.
 * ** Reading ADCPin1, ADCPin2 and ADCPin3 : In this case of reading 3000 samples, 1000 * 3 pin results, each sample takes avg. 3.4us or ~295KHz
 *
 * Setting Read_mode as 5 : With analogReadTurbo() only ADCpin1 result read are returned in a structure to be processed / displayed later.
 * ** Reading a single ADCPin1 : In this case of reading 3000 samples, 3000 * ADCPin1 results, each sample takes avg. 8.24 us per sample or 121KHz sampling rate.
 *
 * It takes a single call to either return 3 ADC pin result or ADCPin1 only. Hence the time for each pin-sample is ~1/3 of a single pin.
 * Not exactly given some extra MBED overhead time and time to copy / calculate results and counting in the loop.
 *
 * background on analogReadTurbo() multi pins read:
 * The read-cycle time for multiple pins (8,29 us / 120Khz) is close to the time for a read-cycle for one pin (8.24us / 121KHz)
 * But in this example we read 3 pins in a single read-cycle, the sample frequency per pin is 3 * 120Khz - MBED overhead = 295khz. Imagine if you need to use all 5....
 *
 *----------------------------------------------------------------------------------------------------------------------
 * analogReadFast()
 *
 * Setting Read_mode as 2 : With analogReadFast() each read the ADC result of one of the 3 ADCPins is returned to be processed / displayed later.
 * ** Reading ADCPin1, ADCPin2 and ADCPin3 : In this case of reading 3000 samples, 1000 (Read_cycles) * 3 pin results, each sample takes avg. 8.29us or ~120KHz.
 *
 * Setting Read_mode as 6 : With analogReadFast() only ADCpin1 result read are returned to be processed / displayed later.
 * ** Reading a single ADCPin1 : In this case of reading 3000 samples, 3000 * ADCPin1 results, each sample takes avg. 6,86 us per sample or ~145KHz sampling rate.
 *
 * Each ADCpin is obtained in a separate call, however all the enabled pins are read by the Apollo3 processor. If you have 3 different pins, they are all enabled
 * but only the result for 1 is returned.
 *
 *----------------------------------------------------------------------------------------------------------------------
 * analogReadSingleFast()
 *
 * Setting Read_mode as 3 : With analogReadSingleFast() each read the ADC result of one of the 3 ADCPins is returned to be processed / displayed later.
 * ** Reading ADCPin1,then ADCPin2 and then ADCPin3 : In this case of reading 3000 samples, 1000 (Read_cycles) * 3 pin results, each sample takes avg. 91us or ~11KHz.
 *
 * Setting Read_mode as 7 : With analogReadSingleFast() only ADCpin1 result read are returned to be processed / displayed later.
 * ** Reading a single ADCPin1 : In this case of reading 3000 samples, 3000 * ADCPin1 results, each sample takes avg. 6,68 us per sample or 149KHz sampling rate.
 *
 * When performing 3 pins, it needs to reset/reinit the ADC slot, as with AnalogRead(). Hence not much benefit over AnalogRead()
 *
 *----------------------------------------------------------------------------------------------------------------------
 * analogRead()
 *
 * Setting Read_mode as 4 : The standard analogRead() each read the ADC result of one of the 3 ADCPins is returned to be processed / displayed later.
 * ** Reading ADCPin1,then ADCPin2 and then ADCPin3 : In that case 3000 samples, 1000 (Read_cycles) * 3 pin results, each sample takes avg. 91us or 11Khz.
 *
 * Setting Read_mode as 8 : The standard analogRead() only ADCpin1 result read are returned to be processed / displayed later.
 * ** Reading a single ADCPin1 : In this case of reading 3000 samples, 3000 * ADCPin1 results, each sample takes avg. 91us or 11Khz.
 *
 * Each time with each ananlogRead() call the ADC slot is reinitialized. So no difference whether you use the same or a different ADCpin
 *
 *----------------------------------------------------------------------------------------------------------------------
 *
 * So you have the choice :
 * Only 1 ADCpin  (Single Fast) 149Khz, (Fast) 132Khz, (Turbo)      121Khz or (Standard) 12Khz
 * 3 ADCPins      (Turbo)       295Khz, (Fast) 120Khz, (Single Fast) 11Khz or (Standard) 11Khz
 *
 *----------------------------------------------------------------------------------------------------------------------
 * struct ADC_Turbo  {
 *   PinName pinName;
 *   uint16_t adc;
 * };
 *
 * @param  res A pointer to a structure of MAXPIN entries for the pins and result
 * @param  Numpin is the number of ADCpins in the structure
 * @param  ReInit if true performs another reinitialize of the ADC slot for the pins
 * @return The pinnumbers stored (same as requested)
 *
 * uint16_t analogReadTurbo(ADC_Turbo *res, uint8_t NumPin, bool ReInit = false);
 *
 * ------------------------------------------------------------------------------------
 *
 * @param  pinNumber the ADCpin (like A0 or A29)
 * @param  ReInit if true performs another reinitialize of the ADC Slot for the pin
 * @return The ADC value read.
 *
 * uint16_t analogReadFast(PinName pinName, bool ReInit = false);
 * uint16_t analogReadSingleFast(PinName pinName, bool ReInit = false);
 *
 */

/**************************************************************************
 * Configuration options
 **************************************************************************/
// Maximum 5 ADC pins maybe selected, in this example we use 3
// if you plan to use more or less adjust the code and different variables
#define ADCpin1 A29
#define ADCpin2 A11
#define ADCpin3 A34

// defined number of pins for correct calculation of results
#define NumPin 3

// define number of ReadCycles. The total number of samples = NumPin * ReadCycles
#define ReadCycles 1000

// define Read_mode :
// read 3 pins       : Turbo mode = 1, Fast mode = 2, SingleFast = 3, Standard mode = 4
// read ADCpin1 only : Turbo mode = 5, Fast mode = 6, SingleFast = 7, Standard mode = 8
uint8_t Read_mode = 1;

// Set to true to enable display the last read ADC results
// does not impact timing calculation
bool ShowLastADCResult = true;

/**************************************************************************
 * NO NEED FOR CHANGES BEYOND THIS POINT
 **************************************************************************/

// place holder for result analogReadFast(), analogReadSingleFast() and analogRead()
uint16_t res1;
uint16_t res2;
uint16_t res3;

// global variables
int loop_count = 0;           // number of read cycles
unsigned long now;            // start time
unsigned long elapsed_time;   // time it took to get the samples

// needed for TURBO mode
ADC_Turbo ADC_result[NumPin];

void setup() {

   Serial.begin(115200);
   Serial.println("\nExample1_V2: Compare Analog Read Speed Test");

   switch(Read_mode){
    case 1:
      Serial.println("Turbo mode multi");
      Read_Turbo();
      break;

    case 2:
      Serial.println("Fast mode multi");
      Read_fast();
      break;

    case 3:
      Serial.println("SingleFast mode multi");
      Read_singlefast();
      break;

    case 4:
      Serial.println("Standard mode multi");
      Read_standard();
      break;

    case 5:
      Serial.println("Turbo mode: AdcPin1 only");
      Read_Turbo1();
      break;

    case 6:
      Serial.println("Fast mode: AdcPin1 only");
      Read_fast1();
      break;

    case 7:
      Serial.println("SingleFast mode: AdcPin1 only");
      Read_singlefast1();
      break;

    case 8:
      Serial.println("Standard mode: AdcPin1 only");
      Read_standard1();
      break;

    default:
      Serial.println("Unknown mode request. freeze");
      while(1);

   }

  //**********************************************
  //** Timing results
  //**********************************************
  Serial.print(ReadCycles * NumPin);
  Serial.print(" samples in ");
  Serial.print(elapsed_time);
  Serial.println(" microseconds");

  Serial.print("Average ");
  float TimePerCycle = (float) elapsed_time / (ReadCycles * NumPin);
  Serial.print(TimePerCycle);
  Serial.print(" microseconds per sample or ");

  Serial.print((float) 1000 / TimePerCycle);
  Serial.println("KHz sampling rate.");
}

void Read_Turbo() {

  // init Turbo with the ADC channels to read
  ADC_result[0].pinName = ADCpin1;
  ADC_result[1].pinName = ADCpin2;
  ADC_result[2].pinName = ADCpin3;

  // !!!!!!!!!!!! MUST BE DONE FIRST !!!!!!!!!!!!!!!
  // Perform a dummy read first to do the init & setup
  analogReadTurbo(&ADC_result[0], NumPin);

  // Now start counting
  now = micros();

  // in Turbo mode we obtain the result for each pin single call
  while (loop_count < ReadCycles) {

    // obtain ONCE all PIN values
    analogReadTurbo(&ADC_result[0], NumPin);
    loop_count++;
  }

  elapsed_time = micros() - now;

  if (ShowLastADCResult) {
    Serial.print("ADC reading Pin 1 ");
    Serial.print(ADC_result[0].adc);

    Serial.print(" Pin 2 ");
    Serial.print(ADC_result[1].adc);

    Serial.print(" Pin 3 ");
    Serial.println(ADC_result[2].adc);
  }
}

void Read_Turbo1() {

  // init Turbo with the ADC channels to read
  ADC_result[0].pinName = ADCpin1;

  // !!!!!!!!!!!! MUST BE DONE FIRST !!!!!!!!!!!!!!!
  // Perform a dummy read first to do the init & setup
  analogReadTurbo(&ADC_result[0], 1);

  // Now start counting
  now = micros();

  // in Turbo mode we obtain the result for each pin single call
  while (loop_count < ReadCycles * NumPin) {

    // obtain ONCE all PIN values
    analogReadTurbo(&ADC_result[0], 1);
    loop_count++;
  }

  elapsed_time = micros() - now;

  if (ShowLastADCResult) {
    Serial.print("ADC reading Pin 1 ");
    Serial.println(ADC_result[0].adc);
  }
}

void Read_fast()
{

  // !!!!!!!!!!!! MUST BE DONE FIRST !!!!!!!!!!!!!!!
  // Perform a dummy read first to do the init & setup
  res1 = analogReadFast(ADCpin1);
  res2 = analogReadFast(ADCpin2);
  res3 = analogReadFast(ADCpin3);

  // Now start counting
  now = micros();

  // in fast mode we obtain the result for each pin in separate call
  while (loop_count < ReadCycles) {
    res1 = analogReadFast(ADCpin1);
    res2 = analogReadFast(ADCpin2);
    res3 = analogReadFast(ADCpin3);

    loop_count++;
  }

  elapsed_time = micros() - now;

  if (ShowLastADCResult) {
    Serial.print("ADC reading Pin 1 ");
    Serial.print(res1);

    Serial.print(" Pin 2 ");
    Serial.print(res2);

    Serial.print(" Pin 3 ");
    Serial.println(res3);
  }
}

void Read_fast1()
{
  // !!!!!!!!!!!! MUST BE DONE FIRST !!!!!!!!!!!!!!!
  // Perform a dummy read first to do the init & setup
  res1 = analogReadFast(ADCpin1);

  // Now start counting
  now = micros();

  // in fast mode we obtain the result for each pin in separate call
  while (loop_count < ReadCycles * NumPin) {
    res1 = analogReadFast(ADCpin1);
    loop_count++;
  }

  elapsed_time = micros() - now;

  if (ShowLastADCResult) {
    Serial.print("ADC reading Pin 1 ");
    Serial.println(res1);
  }
}

void Read_singlefast()
{

  // !!!!!!!!!!!! MUST BE DONE FIRST !!!!!!!!!!!!!!!
  // Perform a dummy read first to do the init & setup
  res1 = analogReadSingleFast(ADCpin1);
  res2 = analogReadSingleFast(ADCpin2);
  res3 = analogReadSingleFast(ADCpin3);

  // Now start counting
  now = micros();

  // in Singlefast mode we obtain the result for each pin in separate call
  while (loop_count < ReadCycles) {
    res1 = analogReadSingleFast(ADCpin1);
    res2 = analogReadSingleFast(ADCpin2);
    res3 = analogReadSingleFast(ADCpin3);
    loop_count++;
  }

  elapsed_time = micros() - now;

  if (ShowLastADCResult) {
    Serial.print("ADC reading Pin 1 ");
    Serial.print(res1);

    Serial.print(" Pin 2 ");
    Serial.print(res2);

    Serial.print(" Pin 3 ");
    Serial.println(res3);
  }
}

void Read_singlefast1()
{

  // !!!!!!!!!!!! MUST BE DONE FIRST !!!!!!!!!!!!!!!
  // Perform a dummy read first to do the init & setup
  res1 = analogReadSingleFast(ADCpin1);

  // Now start counting
  now = micros();

  // in Singlefast mode we obtain the result for each pin in separate call
  while (loop_count < ReadCycles * NumPin) {
    res1 = analogReadSingleFast(ADCpin1);
    loop_count++;
  }

  elapsed_time = micros() - now;

  if (ShowLastADCResult) {
    Serial.print("ADC reading Pin 1 ");
    Serial.println(res1);
  }
}

void Read_standard()
{

  // !!!!!!!!!!!! MUST BE DONE FIRST !!!!!!!!!!!!!!!
  // Perform a dummy read first to do the init & setup
  res1 = analogRead(ADCpin1);
  res2 = analogRead(ADCpin2);
  res3 = analogRead(ADCpin3);

  // Now start counting
  now = micros();

  // in standard mode we obtain the result for each pin in separate call
  while (loop_count < ReadCycles) {
    res1 = analogRead(ADCpin1);
    res2 = analogRead(ADCpin2);
    res3 = analogRead(ADCpin3);

    loop_count++;
  }

  elapsed_time = micros() - now;

  if (ShowLastADCResult) {
    Serial.print("ADC reading Pin 1 ");
    Serial.print(res1);

    Serial.print(" Pin 2 ");
    Serial.print(res2);

    Serial.print(" Pin 3 ");
    Serial.println(res3);
  }
}

void Read_standard1()
{

  // !!!!!!!!!!!! MUST BE DONE FIRST !!!!!!!!!!!!!!!
  // Perform a dummy read first to do the init & setup
  res1 = analogRead(ADCpin1);

  // Now start counting
  now = micros();

  // in standard mode we obtain the result for each pin in separate call
  while (loop_count < ReadCycles * NumPin) {
    res1 = analogRead(ADCpin1);

    loop_count++;
  }

  elapsed_time = micros() - now;

  if (ShowLastADCResult) {
    Serial.print("ADC reading Pin 1 ");
    Serial.println(res1);
  }
}

void loop() {}

/*
 * This sketch demonstrates how analogReadTurbo()or reading multiple ADC pins
 *
 * Version : 1.0
 * Date    : November 2022
 * author  : paulvha
 *
 * This example is only tested on V2.2.1 of the Sparkfun Apollo3 Library. It requires the special version of the CommonAnalog.cpp and common.h
 *
 * struct ADC_Turbo  {
 *   PinName pinName;
 *   uint16_t adc;
 * };
 *
 * @param  res A pointer to a structure of MAXPIN entries for the pins and result
 * @param  NumPin is the number of ADCpins in the structure
 * @param  ReInit if true performs another reinitialize of the ADC slot for the pins
 * @return The pinnumbers stored (same as requested)
 *
 * uint16_t analogReadTurbo(ADC_Turbo *res, uint8_t NumPin, bool ReInit = false);
 */

// choose the right pin for your board
#define ADCpin1 A29
#define ADCpin2 A11
#define ADCpin3 A34

// defined number of pins for correct calculation of results
#define NumPin 3

int loop_count = 0;           // number of read cycles
unsigned long now;            // start time
unsigned long elapsed_time;   // time it took to get the samples

void setup() {

    Serial.begin(115200);
    Serial.println("Example2_V2 : Turbo Analog Read Speed Test");

    // define for 3 pins
    ADC_Turbo ADC_result[NumPin];

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
    while (loop_count < 1000) {

      // obtain ONCE all PIN values
      analogReadTurbo(&ADC_result[0], NumPin);
      loop_count++;
    }

    elapsed_time = micros() - now;

    // display last ADC results
    Serial.print("ADC reading Pin 1 ");
    Serial.print(ADC_result[0].adc);

    Serial.print(" Pin 2 ");
    Serial.print(ADC_result[1].adc);

    Serial.print(" Pin 3 ");
    Serial.println(ADC_result[2].adc);

    // display timing
    Serial.print(loop_count * NumPin);
    Serial.print(" samples in ");
    Serial.print(elapsed_time);
    Serial.println(" microseconds");

    // display timing result
    Serial.print("Average ");
    float TimePerCycle = (float) elapsed_time / (loop_count * NumPin);
    Serial.print(TimePerCycle);
    Serial.print(" microseconds per sample or ");

    Serial.print((float) 1000 / TimePerCycle);
    Serial.println("KHz sampling rate.");
}

void loop() {}

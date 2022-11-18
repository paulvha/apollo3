/*
 * This sketch demonstrates how analogFast()or reading multiple ADC pins
 *
 * Version : 1.0
 * Date    : November 2022
 * author  : paulvha
 *
 * This example is only tested on V2.2.1 of the Sparkfun Apollo3 Library. It requires the special version of the CommonAnalog.cpp and common.h
 *
 * @param  pinName the ADCpin (like A0 or A29)
 * @param  ReInit if true performs another reinitialize of the ADC Slot for the pin
 * @return The ADC value read.
 *
 * uint16_t analogReadFast(PinName pinName, bool ReInit = false);
 */

// choose the right pin for your board
#define ADCpin1 A29
#define ADCpin2 A11
#define ADCpin3 A34

int loop_count = 0;           // number of read cycles
unsigned long now;            // start time
unsigned long elapsed_time;   // time it took to get the samples

void setup() {

    Serial.begin(115200);
    Serial.println("Example3_V2 : Fast Analog Read Speed Test");

    // !!!!!!!!!!!! MUST BE DONE FIRST !!!!!!!!!!!!!!!
    // Perform a dummy read first to do the init & setup
    uint16_t res1 = analogReadFast(ADCpin1);
    uint16_t res2 = analogReadFast(ADCpin2);
    uint16_t res3 = analogReadFast(ADCpin3);

    // Now start counting
    now = micros();

    // in fast mode we obtain the result for each pin in separate call
    while (loop_count < 1000) {
      res1 = analogReadFast(ADCpin1);
      res2 = analogReadFast(ADCpin2);
      res3 = analogReadFast(ADCpin3);

      loop_count++;
    }

    elapsed_time = micros() - now;

    // display ADC readings
    Serial.print("ADC reading Pin 1 ");
    Serial.print(res1);

    Serial.print(" Pin 2 ");
    Serial.print(res2);

    Serial.print(" Pin 3 ");
    Serial.println(res3);

    // display timing
    Serial.print(loop_count * 3);
    Serial.print(" samples in ");
    Serial.print(elapsed_time);
    Serial.println(" microseconds");

    // display timing result
    Serial.print("Average ");
    float TimePerCycle = (float) elapsed_time / (loop_count * 3);
    Serial.print(TimePerCycle);
    Serial.print(" microseconds per sample or ");

    Serial.print((float) 1000 / TimePerCycle);
    Serial.println("KHz sampling rate.");
}

void loop() {}

/*
 * This sketch demonstrates how analogSingleFast()or reading multiple ADC pins 
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
 * uint16_t analogReadSingleFast(PinName pinName, bool ReInit = false);
*/
 
 // analogReadSingleFast is mend to be used for fast reading SAME pin everytime
  
/* analogReadSingleFast() is a modified analogRead() optimized to perform a fast reading of a SINGLE ADCpin. It can achieve a very high
 * frequency, but only on a single ADCPin. When performing 3 pins, it needs to reset/reinit the ADC slot, as with analogRead(). 
 * Hence not much benefit over analogReadSingleFast()
 * 
 */

// choose the right pin for your board
#define ADCpin1 A29

int loop_count = 0;           // number of read cycles
unsigned long now;            // start time
unsigned long elapsed_time;   // time it took to get the samples

void setup() {
    
    Serial.begin(115200);
    Serial.println("Example4_V2 : SingleFast Analog Read Speed Test");

    // !!!!!!!!!!!! MUST BE DONE FIRST !!!!!!!!!!!!!!!
    // Perform a dummy read first to do the init & setup
    // the first time takes much longer
    uint16_t res1 = analogReadSingleFast(ADCpin1);

     // Now start counting
    now = micros();
  
    // in fast mode we obtain the result for each pin in separate call
    while (loop_count < 1000 * 3) {
      res1 = analogReadSingleFast(ADCpin1);
      loop_count++;
    }
    
    elapsed_time = micros() - now;
    
    // display ADC readings
    Serial.print("ADC reading Pin 1 ");
    Serial.println(res1);

    // display timing
    Serial.print(loop_count); 
    Serial.print(" samples in "); 
    Serial.print(elapsed_time); 
    Serial.println(" microseconds");

    // display timing result
    Serial.print("Average ");
    float TimePerCycle = (float) elapsed_time / loop_count;
    Serial.print(TimePerCycle); 
    Serial.print(" microseconds per sample or ");
  
    Serial.print((float) 1000 / TimePerCycle);
    Serial.println("KHz sampling rate.");
}

void loop() {}

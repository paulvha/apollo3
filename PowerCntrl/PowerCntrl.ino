/*
 * This sketch can be used to better understand, debug the power control of a Apollo3
 * 
 * There is a document in the same folder that explains in details about the registers,
 * what and how memory or peripherals can be put in deep sleep as well as the consequences
 * are and aspects to consider.
 * 
 * In the tab DisplayPowerControl are different calls to display the setting from different
 * power control registers
 * 
 * display_buck()
 * display_peripherals()
 * display_memory()
 * display_events()
 * 
 * Version 1.0 / February 2023 / paulvha
 */

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Power Control Registers");

  // bucks
  display_buck();
  
  // peripherals
  display_peripherals();

  // memory
  display_memory();

  //events
  display_events();
}

void loop() {
  // put your main code here, to run repeatedly:

}

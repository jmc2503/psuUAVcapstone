#include "Adafruit_VL53L0X.h"

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
int value;
int bufferSize = 5;
float buffer[5];
unsigned long lastTime;



void setup() {
  Serial.begin(115200);

  // wait until serial port opens for native USB devices
  while (! Serial) {
    delay(1);
  }
  
  Serial.println("Adafruit VL53L0X test");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
  // power 
  Serial.println(F("VL53L0X API Simple Ranging example\n\n")); 
}


void loop() {
  VL53L0X_RangingMeasurementData_t measure;
    
  //Serial.print("Reading a measurement... ");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    value = measure.RangeMilliMeter;
   // Serial.print("Distance (mm): "); 
   Serial.println(value);
   shiftArrayRight(buffer, bufferSize, value);
   unsigned long period = FindPeaks(buffer);
   if(period != 0){
    //Serial.println(period);
   }
  } else {
    //Serial.println(" out of range ");
    //Serial.println(-1);
  }
    
  delay(5);
}

unsigned long FindPeaks(float buffer[]){

  float mp = buffer[2];

  unsigned long period = 0;

  //Look for peak
  if(mp > buffer[0] && mp > buffer[1] && mp > buffer[3] && mp > buffer[4]){
    unsigned long currTime = millis();

    period = currTime - lastTime;
    lastTime = currTime;

  }

  return period;

}

void shiftArrayRight(float buffer[], int size, float newValue) {
    
    for (int i = size - 1; i > 0; i--) {
        buffer[i] = buffer[i - 1];
    }
    // Place the new value in the leftmost spot
    buffer[0] = newValue;
}

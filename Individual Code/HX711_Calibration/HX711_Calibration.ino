/*
 Example using the SparkFun HX711 breakout board with a scale
 By: Nathan Seidle
 SparkFun Electronics
 Date: November 19th, 2014
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

 This is the calibration sketch. Use it to determine the calibration_factor that the main example uses. It also
 outputs the zero_factor useful for projects that have a permanent mass on the scale in between power cycles.

 Setup your scale and start the sketch WITHOUT a weight on the scale
 Once readings are displayed place the weight on the scale
 Press +/- or a/z to adjust the calibration_factor until the output readings match the known weight
 Use this calibration_factor on the example sketch

 This example assumes pounds (lbs). If you prefer kilograms, change the Serial.print(" lbs"); line to kg. The
 calibration factor will be significantly different but it will be linearly related to lbs (1 lbs = 0.453592 kg).

 Your calibration factor may be very positive or very negative. It all depends on the setup of your scale system
 and the direction the sensors deflect from zero state
 This example code uses bogde's excellent library: https://github.com/bogde/HX711
 bogde's library is released under a GNU GENERAL PUBLIC LICENSE
 Arduino pin 2 -> HX711 CLK
 3 -> DOUT
 5V -> VCC
 GND -> GND

 Most any pin on the Arduino Uno will be compatible with DOUT/CLK.

 The HX711 board can be powered from 2.7V to 5V so the Arduino 5V power should be fine.

*/

#include "HX711.h"
#define DOUT1 18
#define CLK1 19

#define DOUT2 17
#define CLK2 16

#define DOUT3 25
#define CLK3 24

HX711 scale1;
HX711 scale2;
HX711 scale3;


//for load cell 1
float calibration_factor1 = 472100.00;
//thing1: 471500.00
//thing1 better: 472100.00


//for load cell 2
float calibration_factor2 = 494800.00;
//thing2: 490800.00
//thing2 better: 494800.00

//for load cell 3
float calibration_factor3 = 487700.00;
//thing3: 497200.00
//thing3 better: 487700.00 


void setup() {
  Serial.begin(9600);
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");

  scale1.begin(DOUT1, CLK1);
  scale1.set_scale();
  scale1.tare(); //Reset the scale to 0

  scale2.begin(DOUT2, CLK2);
  scale2.set_scale();
  scale2.tare(); //Reset the scale to 0

  scale3.begin(DOUT3, CLK3);
  scale3.set_scale();
  scale3.tare(); //Reset the scale to 0

  long zero_factor1 = scale1.read_average(); //Get a baseline reading
  long zero_factor2 = scale2.read_average(); //Get a baseline reading
  long zero_factor3 = scale3.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor1);
  Serial.println(zero_factor2);
  Serial.println(zero_factor3);

}

void loop() {

  scale1.set_scale(calibration_factor1); //Adjust to this calibration factor
  scale2.set_scale(calibration_factor2); //Adjust to this calibration factor
  scale3.set_scale(calibration_factor3); //Adjust to this calibration factor

  float scale1_reading = scale1.get_units();
  float scale2_reading = scale2.get_units();
  float scale3_reading = scale3.get_units();
  float sum = scale1_reading + scale2_reading + scale3_reading;


  Serial.print("Reading: ");
  Serial.print(scale1_reading, 5);
  Serial.print(", ");
  Serial.print(scale2_reading, 5);
  Serial.print(", ");
  Serial.print(scale3_reading, 5);
  Serial.print(", ");
  Serial.print(sum);
  Serial.print(" lbs"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
  Serial.print(" calibration_factors: ");
  Serial.print(calibration_factor1);
  Serial.print(", ");
  Serial.print(calibration_factor2);
  Serial.print(", ");
  Serial.print(calibration_factor3);
  Serial.println();

  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == '1')
      calibration_factor1 += 100;
    else if(temp == '!')
      calibration_factor1 -= 100;
    else if(temp == '2')
      calibration_factor2 += 100;
    else if(temp == '@')
      calibration_factor2 -= 100;
    else if(temp == '3')
      calibration_factor3 += 100;
    else if(temp == '#')
      calibration_factor3 -= 100;
  }
}
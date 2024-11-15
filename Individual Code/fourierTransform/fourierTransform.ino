#include "arduinoFFT.h"
#include <Arduino_LSM6DSOX.h>

const uint16_t SAMPLES = 64;       
const double SAMPLING_FREQUENCY = 50; 



float vReal[SAMPLES];
float vImag[SAMPLES];
unsigned long sampling_period_us;
unsigned long microseconds;

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

void setup() {
  Serial.begin(115200);
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  
  Serial.println("Gyroscope FFT Analysis");
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
}

void loop() {

  for (int i = 0; i < SAMPLES; i++) {
    microseconds = micros();
    
    float gx, gy, gz;
    IMU.readGyroscope(gx, gy, gz); 
    vReal[i] = gy;
    vImag[i] = 0; 

    // Wait for the next sample
    while (micros() - microseconds < sampling_period_us) {}
  }

  // Perform FFT
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

  double peakFrequency = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);
  
  Serial.print("Peak Frequency: ");
  Serial.print(peakFrequency);
  Serial.println(" Hz");
  
  for (int i = 0; i < (SAMPLES / 2); i++) {
    Serial.print("Bin ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(vReal[i]);
  }
  
  delay(1000);
}

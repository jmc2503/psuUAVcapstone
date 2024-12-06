#include <Arduino_LSM6DSOX.h>
//#include <SD.H>

//FLOW OF USING
//1. GET GYROSCOPE READY
//2. PRESS SPACE
//3. MEASURE FOR 10s
//  3.1 FIll in buffer
//  3.2 Look for peaks/zero crossings
//4. AVERAGE PERIODS MEASURED DURING THAT TIME
//5. PLUG INTO EQUATION

//GENERAL
unsigned long lastTime = 0;

//EQUATION CONSTANTS FOR EQ1
float PLATFORM_WEIGHT = 0;
float PLATFORM_DISTANCE = 0;

//PERIOD DETECTION
int bufferSize = 5;
float buffer[5];
float bufferNorm[5];
bool wasPositive = false;

float prevX = 0;
float prevY = 0;
float prevZ = 0;
float alpha = 0.1;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");

    while (1);
  }

  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Gyroscope in degrees/second");
  Serial.println("X\tY\tZ");

  switch (sox.getAccelDataRate()) {
  case LSM6DS_RATE_SHUTDOWN:
    Serial.println("0 Hz");
    break;
  case LSM6DS_RATE_12_5_HZ:
    Serial.println("12.5 Hz");
    break;
  case LSM6DS_RATE_26_HZ:
    Serial.println("26 Hz");
    break;
  case LSM6DS_RATE_52_HZ:
    Serial.println("52 Hz");
    break;
  case LSM6DS_RATE_104_HZ:
    Serial.println("104 Hz");
    break;
  case LSM6DS_RATE_208_HZ:
    Serial.println("208 Hz");
    break;
  case LSM6DS_RATE_416_HZ:
    Serial.println("416 Hz");
    break;
  case LSM6DS_RATE_833_HZ:
    Serial.println("833 Hz");
    break;
  case LSM6DS_RATE_1_66K_HZ:
    Serial.println("1.66 KHz");
    break;
  case LSM6DS_RATE_3_33K_HZ:
    Serial.println("3.33 KHz");
    break;
  case LSM6DS_RATE_6_66K_HZ:
    Serial.println("6.66 KHz");
    break;
  }
}

void loop() {
  float x, y, z;

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x, y, z);

    Serial.print(x);
    Serial.print('\t');
    Serial.print(y);
    Serial.print('\t');
    Serial.print(z);
    Serial.println();

    //DETECT PEAK and Filtering
    // float totalSum = shiftArrayRight(buffer, bufferSize, x);
    // float avgSum = totalSum / bufferSize;
    // shiftArrayRight(bufferNorm, bufferSize, avgSum);
    // unsigned long period = FindPeaks(bufferNorm, currTime);



    // //Graph Manipulation
    // Serial.print(avgSum);
    // Serial.print('\t');
    // Serial.print(3);
    // Serial.print('\t');
    // Serial.print(period);
    // Serial.print('\t');
    // Serial.println(-3);

    delay(50);
    
  }
}

unsigned long FindPeaks(float buffer[], unsigned long currTime){

  float mp = buffer[2];

  unsigned long period = 0;

  //Look for peak
  if(mp > buffer[0] && mp > buffer[1] && mp > buffer[3] && mp > buffer[4]){


    period = currTime - lastTime;
    lastTime = currTime;

  }

  return period;

}

float shiftArrayRight(float buffer[], int size, float newValue) {
    
  float totalSum = 0;

    for (int i = size - 1; i > 0; i--) {
        totalSum += buffer[i-1];
        buffer[i] = buffer[i - 1];
    }
    // Place the new value in the leftmost spot
    buffer[0] = newValue;
    return totalSum + newValue;
}

float EquationOne(unsigned long period){
  
  return 0.0;
}

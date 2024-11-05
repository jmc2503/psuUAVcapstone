#include <Arduino_LSM6DSOX.h>
//#include <SD.H>

//GENERAL
unsigned long lastTime = 0;

//EQUATION CONSTANTS FOR EQ1
float UAV_WEIGHT = 0; //units
float PLATFORM_WEIGHT = 0; //units
float PERIOD_PLATFORM = 0; //units
float PERIOD_COMBINED = 0; //units
float DISTANCE_CoG_TO_PLATFORM = 0; //units
float DISTANCE_CoG_TO_UAV = 0; //units

//EQUATION 2
float previousGyro = 0;
float angle = 0;

//PERIOD DETECTION
int bufferSize = 5;
float buffer[5];

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
}

void loop() {
  float x, y, z;
  unsigned long currTime = millis();
  float deltaTime = (currentTime - previousTime)  1000.0;

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x, y, z);

    //ANGULAR VELOCITY
    Serial.print(x);
    Serial.print('\t');
    //Serial.print(y);
    //Serial.print('\t');
    //Serial.println(z);

    //ANGLE
    angle += x * deltaTime;

    //ANGULAR ACCELERATION
    angularAcceleration = (x - previousGyro) / deltaTime;

    //DETECT PEAK
    shiftArrayRight(buffer, bufferSize, x);
    unsigned long period = FindPeaks(buffer, currTime);
    
    Serial.print('\t');
    // if(period != 0){
    //   //Serial.println(period);
    // }

    Serial.print(angle);
    Serial.print("\t");
    Serial.print(angularAcceleration);
    Serial.println("\t");

    //UPDATE VALUES
    lastTime = currTime;
    previousGyro = x;

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

void shiftArrayRight(float buffer[], int size, float newValue) {
    
    for (int i = size - 1; i > 0; i--) {
        buffer[i] = buffer[i - 1];
    }
    // Place the new value in the leftmost spot
    buffer[0] = newValue;
}

float EquationOne(unsigned long period){
  
  return 0.0;
}

/*
template<typename... Args>
void writeToSD(Args... args) {
  File dataFile = SD.open("data.csv", FILE_WRITE); 
  if (dataFile) {
    std::vector<float> values = {static_cast<float>(args)...};
    for (size_t i = 0; i < values.size(); i++) {
      dataFile.print(values[i]);
      if (i < values.size() - 1) {
        dataFile.print(",");
      }
    }
    dataFile.println();
    dataFile.close();
  } else {
    Serial.println("File not open.");
  }
}
*/

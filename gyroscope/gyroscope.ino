#include <Arduino_LSM6DSOX.h>
#include <SD.H>
int bufferSize = 5;
float buffer[5];
unsigned long lastTime;

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

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x, y, z);

    Serial.print(x);
    Serial.print('\t');
    Serial.print(y);
    Serial.print('\t');
    Serial.println(z);

    //Detect Peak
    shiftArrayRight(buffer, bufferSize, x);
    unsigned long period = FindPeaks(buffer);
    
    Serial.print('\t');
    // if(period != 0){
    //   //Serial.println(period);
    // }

  }
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

template<typename... Args>
void writeToSD(Args... args) {
  File dataFile = SD.open("data.csv", FILE_WRITE); // Open file each time
  if (dataFile) {
    std::vector<float> values = {static_cast<float>(args)...}; // Store values in a vector
    for (size_t i = 0; i < values.size(); i++) {
      dataFile.print(values[i]);
      if (i < values.size() - 1) {
        dataFile.print(",");
      }
    }
    dataFile.println();
    dataFile.close(); // Close file after each write
  } else {
    Serial.println("File not open.");
  }
}

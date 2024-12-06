#include <Adafruit_LSM6DSOX.h>

const int duration = 10;
const int sampleRate = 100;
const int sampleTime = 1000 / sampleRate;
const int arraySize = duration * sampleRate;

float sensorData[arraySize];
unsigned long timeData[arraySize];
int sample_index = 0;
bool isRecording = false;
unsigned long startTime;

Adafruit_LSM6DSOX sox;

void setup() {
  Serial.begin(9600);


  if (!sox.begin_I2C()) {
    Serial.println("Failed to find LSM6DSOX chip");
    while (1) {
      delay(10);
    }
  }

  Serial.println("LSM6DSOX Found!");

  // sox.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS );
  Serial.print("Gyro range set to: ");
  switch (sox.getGyroRange()) {
  case LSM6DS_GYRO_RANGE_125_DPS:
    Serial.println("125 degrees/s");
    break;
  case LSM6DS_GYRO_RANGE_250_DPS:
    Serial.println("250 degrees/s");
    break;
  case LSM6DS_GYRO_RANGE_500_DPS:
    Serial.println("500 degrees/s");
    break;
  case LSM6DS_GYRO_RANGE_1000_DPS:
    Serial.println("1000 degrees/s");
    break;
  case LSM6DS_GYRO_RANGE_2000_DPS:
    Serial.println("2000 degrees/s");
    break;
  case ISM330DHCX_GYRO_RANGE_4000_DPS:
    break; // unsupported range for the DSOX
  }


  sox.setGyroDataRate(LSM6DS_RATE_3_33K_HZ);
  Serial.print("Gyro data rate set to: ");
  switch (sox.getGyroDataRate()) {
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
  
  delay(4000);

  Serial.println(arraySize);

  Serial.println("Insert spacebar for recording begin hooray");  
}

void loop() {

  sensors_event_t gyro;
  sensors_event_t accel;
  sensors_event_t temp;

  if (Serial.available() > 0) {
    char input = Serial.read();
    if (input == ' ' && !isRecording) {
        Serial.println("Recording started!!!");
        isRecording = true;
        startTime = millis(); // Record the starting time
        sample_index = 0; // Reset index for new recording
      }
  }

  if(isRecording){
    if(millis() - startTime < duration * 1000){
      sox.getEvent(&accel, &gyro, &temp);
      sensorData[sample_index] = gyro.gyro.y;
      timeData[sample_index] = millis();
      sample_index++;
      delay(1000/sampleRate);
    }
    else{
      isRecording = false;

      float filtered[arraySize];
      for (int i = 0; i < arraySize; i++) {
        filtered[i] = sensorData[i];
      }

      moving_avg_filter(sensorData, 5, filtered);

      float period = get_period(filtered);
      Serial.print("Period: ");
      Serial.println(period);
    }
  }
}

float get_period(float values[]){

  int peakSize = 5;
  bool foundFirstPeak = false;
  unsigned long lastTime = 0;

  int numPeriods = 0;
  float periodSum = 0;

  for(int i = peakSize; i < arraySize - peakSize; i++){
    // int peakRangeSize = 2 * peakSize + 1;
    // float peakRange[peakRangeSize];
    // get_peak_range(i, peakRangeSize, values, peakRange);
    
    if(detect_peak(values, i, peakSize)){
      if(foundFirstPeak == false){
        lastTime = timeData[i];
        foundFirstPeak = true;
      }
      else{
        unsigned long currTime = timeData[i];
        
        Serial.println(currTime - lastTime);
        
        periodSum += currTime - lastTime;
        numPeriods++;
        lastTime = currTime;
      }
    }
  }

  return periodSum / numPeriods;

}

bool detect_peak(float values[], int centerIndex, int size) {
  float middleValue = values[centerIndex];

  for (int i = centerIndex - size; i <= centerIndex + size; i++) {
    if (i < 0 || i >= arraySize || i == centerIndex) {
      continue;
    }
    if (values[i] >= middleValue) {
      return false; // Not a peak if any surrounding value is greater or equal
    }
  }
  return true; // It's a peak if no surrounding value is greater
}

void moving_avg_filter(float values[], int size, float filtered[]){
  int windowSize = 2 * size + 1;

  float sum = 0;
  for (int i = 0; i < windowSize; i++) {
    sum += values[i];
  }

  filtered[size] = sum / windowSize;

  for (int i = size + 1; i < arraySize - size; i++) {
    sum = sum - values[i - size - 1] + values[i + size];
    filtered[i] = sum / windowSize;
  }
}

// void get_peak_range(int location, int size, float values[], float peakRange[]) {
//   int peakIndex = 0;

//   for (int i = location - size; i <= location + size; i++) {
//     if (i >= 0 && i < arraySize) {
//       peakRange[peakIndex] = values[i];
//     } else {
//       peakRange[peakIndex] = 0;
//     }
//     peakIndex++;
//   }
// }

// bool detect_peak(float values[], int length){
//   int middleIndex = length / 2;
//   int mp = values[middleIndex];

//   for(int i = 0; i < length; i++){
//     if(values[i] > mp){
//       return false;
//     }
//   }

//   return true;
// }


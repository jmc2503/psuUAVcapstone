#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <string.h> // For strcpy and strncpy
#include <Arduino_LSM6DSOX.h>

//****************SCREEN VARIABLES*******************

// Initialize the LCD object for a 20x4 display with I2C address 0x27
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Keypad setup
const byte ROWS = 4; // Four rows
const byte COLS = 3; // Three columns (1-9, *, and #)
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {3, 8, 7, 5}; // Connect keypad ROW1, ROW2, ROW3, ROW4 to Teensy pins
byte colPins[COLS] = {4, 2, 6};    // Connect keypad COL1, COL2, COL3 to Teensy pins
Keypad customkeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int menuState = 0; // 0: Main menu, 1: Center of Gravity, 2: Inertia, 3: CoG Screen 2, 4: Next CoG Screen, 5: CoG Screen 2 Z
int cogMode = 0;   // 0: none, 1: xy, 2: zy
const int COG_MODE_XY = 1;
const int COG_MODE_ZY = 2;

String x_COG = ""; // Variable to store user input for x_COG
String z_COG = ""; // Variable to store user input for z_COG
String Final_x_COG = ""; // Variable to store saved Final value for x_COG
String Final_z_COG = ""; // Variable to store saved Final value for z_COG
bool decimalEntered = false; // Flag to check if a decimal has been entered
bool x_oscillated = false; // Track if X oscillation is done
bool y_oscillated = false; // Track if Y oscillation is done
bool z_oscillated = false; // Track if Z oscillation is done
String MMI_final_xy = ""; // Holds final calculated value for Inertia XY
String MMI_final_yz = ""; // Holds final calculated value for Inertia YZ

float xPeriod = 0;

//************INERTIA VARIABLES**************

const int duration = 10; // How long the test runs for
const int sampleRate = 100;
const int sampleTime = 1000 / sampleRate;
const int arraySize = duration * sampleRate;

float sensorData[arraySize]; // Readings from the gyroscope
unsigned long timeData[arraySize]; // What time those readings occur for calculating period
int sample_index = 0;
bool isRecording = false;
bool oscillationDone = false;
unsigned long startTime;

void setup() {
  Serial.begin(9600);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");

    while (1);
  }
  
  lcd.init();         // Initialize the LCD
  lcd.backlight();    // Turn on the backlight
  lcd.clear();        // Clear the screen
  displayMainMenu();  // Display the main menu on startup
}

void loop() {
  char key = customkeypad.getKey(); // Continuously read key press from the keypad

  if (key != NO_KEY) { // If a key is pressed
    if (menuState == 0) {
      handleMainMenu(key); // Handle main menu key presses
    } 
    else if (menuState == 1) {
      handleCenterOfGravityMenu(key); // Handle Center of Gravity menu key presses
    }
    else if (menuState == 2) {
      handleInertiaMenu(key); // Handle Inertia menu key presses
    }
    else if (menuState == 3) {
      handleCoGScreen2Menu(key); // Handle CoG Screen 2 key presses
    }
    else if (menuState == 4) {
      if (cogMode == COG_MODE_XY) {
        handleCoGScreen3XY(key);
      } else if (cogMode == COG_MODE_ZY) {
        handleCoGScreen3ZY(key);
      }
    }
    else if (menuState == 5) {
      handleCoGScreen2Z(key); // Handle CoG Screen 2 Z key presses
    }
    else if (menuState == 6) {
      handleInertiaXYScreen(key); // Handle Inertia XY screen key presses
    }
    else if (menuState == 7) {
      handleInertiaYZScreen(key); // Handle Inertia YZ screen key presses
    }
    else if (menuState == 8) {
      handleInertiaXOscillate(key); // Handle X oscillation key presses
    }
    else if (menuState == 9) {
      handleInertiaYOscillate(key); // Handle Y oscillation key presses
    }
    else if (menuState == 10) {
      handleInertiaZOscillate(key); // Handle Z oscillation key presses
    }
  }

  //Inertia Loop
  if(menuState == 8){
    if(!oscillationDone){
      PerformOscillation();
    }
  }

}


void handleMainMenu(char key) {
  if (key == '1') {
    displayCenterOfGravityMenu();
    menuState = 1; // Set menuState to Center of Gravity menu
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    displayMainMenu(); // Redisplay the main menu
  }
}

void handleCenterOfGravityMenu(char key) {
  if (key == '#') {
    displayMainMenu(); // Go back to main menu
    menuState = 0;     // Reset to main menu state
  } else if (key == '1') {
    cogMode = COG_MODE_XY; // Set mode to xy
    cog_screen_2xy(); // Display CoG Screen 2
    menuState = 3;    // Set menuState to CoG Screen 2
  } else if (key == '2') {
    cogMode = COG_MODE_ZY; // Set mode to zy
    cog_screen_2zy(); // Display CoG Screen 2 Z
    menuState = 5;    // Set menuState to CoG Screen 2 Z
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    displayCenterOfGravityMenu(); // Redisplay Center of Gravity menu
  }
}

void handleInertiaMenu(char key) {
  if (key == '#') {
    displayMainMenu(); // Go back to main menu
    menuState = 0;     // Reset to main menu state
  } else if (key == '1') {
    inertia_xy_screen(); // Show Inertia XY screen
    menuState = 6;
  } else if (key == '2') {
    inertia_yz_screen(); // Show Inertia YZ screen
    menuState = 7;
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    displayInertiaMenu(); // Redisplay Inertia menu
  }
}

void handleCoGScreen2Menu(char key) {
  if (key == '#') {
    x_COG = "";             // Clear past number
    decimalEntered = false; // Reset decimal flag
    displayCenterOfGravityMenu(); // Go back to Center of Gravity menu
    menuState = 1;          // Set menuState back to Center of Gravity menu
  } else if (key == '*') {
    if (!decimalEntered) {
      if (x_COG.length() < 13) { // Allow space for the decimal and "in"
        x_COG += '.';
        decimalEntered = true;
        lcd.setCursor(0, 2);
        lcd.print(x_COG);
        lcd.setCursor(18, 2);
        lcd.print("in");
      }
    } else {
      // Store the value in Final_x_COG and show "Value saved"
      Final_x_COG = x_COG;
      lcd.clear();
      lcd.print("Value saved");
      delay(1000);
      cog_screen_3_xy(); // Go to the next screen for further actions
      menuState = 4; // Move to the next menu state
    }
  } else if (isdigit(key)) { // Capture numeric input
    if (x_COG.length() < 14) { // Limit input length to fit on the display
      x_COG += key;
      lcd.setCursor(0, 2);
      lcd.print(x_COG);
      lcd.setCursor(18, 2);
      lcd.print("in");
    }
  }
}

void handleCoGScreen2Z(char key) {
  if (key == '#') {
    z_COG = "";             // Clear past number
    decimalEntered = false; // Reset decimal flag
    displayCenterOfGravityMenu(); // Go back to Center of Gravity menu
    menuState = 1;          // Set menuState back to Center of Gravity menu
  } else if (key == '*') {
    if (!decimalEntered) {
      if (z_COG.length() < 13) { // Allow space for the decimal and "in"
        z_COG += '.';
        decimalEntered = true;
        lcd.setCursor(0, 2);
        lcd.print(z_COG);
        lcd.setCursor(18, 2);
        lcd.print("in");
      }
    } else {
      // Store the value in Final_z_COG and show "Value saved"
      Final_z_COG = z_COG;
      lcd.clear();
      lcd.print("Value saved");
      delay(1000);
      cog_screen_3_zy(); // Go to the next screen
      menuState = 4; // Move to the next menu state
    }
  } else if (isdigit(key)) { // Capture numeric input
    if (z_COG.length() < 14) { // Limit input length to fit on the display
      z_COG += key;
      lcd.setCursor(0, 2);
      lcd.print(z_COG);
      lcd.setCursor(18, 2);
      lcd.print("in");
    }
  }
}

void handleCoGScreen3XY(char key) {
  if (key == '#') {
    cog_screen_2xy(); // Go back to previous screen
    menuState = 3;
  } else if (key == '1') {
    if (Final_z_COG.length() == 0) { // Check if z_COG has been saved
      cog_screen_2zy(); // Redirect to z_COG input if missing
      menuState = 5;
    } else {
      displayInertiaMenu(); // Access inertia menu if both values are saved
      menuState = 2;
    }
  } else if (key == '*') {
    lcd.clear();
    lcd.print("Value saved");
    delay(1000);
    cog_screen_3_xy(); // Stay on the same screen
  }
}

void handleCoGScreen3ZY(char key) {
  if (key == '#') {
    cog_screen_2zy(); // Go back to previous screen
    menuState = 5;
  } else if (key == '1') {
    if (Final_x_COG.length() == 0) { // Check if x_COG has been saved
      cog_screen_2xy(); // Redirect to x_COG input if missing
      menuState = 3;
    } else {
      displayInertiaMenu(); // Access inertia menu if both values are saved
      menuState = 2;
    }
  } else if (key == '*') {
    lcd.clear();
    lcd.print("Value saved");
    delay(1000);
    cog_screen_3_zy(); // Stay on the same screen
  }
}

void handleInertiaXYScreen(char key) {
  if (key == '#') {
    displayInertiaMenu(); // Go back to inertia menu
    menuState = 2;
  } else if (key == '1') { // Handle X oscillation
    inertia_x_oscillate(); // Go to the X oscillation screen
    menuState = 8; // Update menu state for X oscillation
  } else if (key == '2') { // Handle Y oscillation
    inertia_y_oscillate(); // Go to the Y oscillation screen
    menuState = 9; // Update menu state for Y oscillation
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    inertia_xy_screen(); // Redisplay the Inertia XY screen
  }
}

void handleInertiaYZScreen(char key) {
  if (key == '#') {
    displayInertiaMenu(); // Go back to inertia menu
    menuState = 2;
  } else if (key == '1') { // Handle Y oscillation
    inertia_y_oscillate(); // Go to the Y oscillation screen
    menuState = 9; // Update menu state for Y oscillation
  } else if (key == '2') { // Handle Z oscillation
    inertia_z_oscillate(); // Go to the Z oscillation screen
    menuState = 10; // Update menu state for Z oscillation
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    inertia_yz_screen(); // Redisplay the Inertia YZ screen
  }
}

void handleInertiaXOscillate(char key) {
  if (key == '#') {
    inertia_xy_screen(); // Go back to Inertia XY screen
    menuState = 6;
  } else if (key == '1') { // Save the X oscillation
    x_oscillated = true;
    lcd.clear();
    lcd.print("X Oscillation Saved");
    delay(1000);
    if (!y_oscillated) { // Check if Y oscillation is pending
      inertia_y_oscillate(); // Redirect to Y oscillate
      menuState = 8;
    } else {
      MMI_final_xy = "123.45"; // Replace with actual calculation
      displayResultsScreen("XY", MMI_final_xy, MMI_final_yz); // Show results for Inertia XY
      menuState = 10; // Transition to results screen state
    }
  }
}

void handleInertiaYOscillate(char key) {
  if (key == '#') {
    inertia_xy_screen(); // Go back to Inertia XY screen
    menuState = 6;
  } else if (key == '1') { // Save the Y oscillation
    y_oscillated = true;
    lcd.clear();
    lcd.print("Y Oscillation Saved");
    delay(1000);
    if (!x_oscillated) { // Check if X oscillation is pending
      inertia_x_oscillate(); // Redirect to X oscillate
      menuState = 8;
    } else {
      MMI_final_xy = "123.45"; // Replace with actual calculation
      displayResultsScreen("XY", MMI_final_xy, MMI_final_yz); // Show results for Inertia XY
      menuState = 10; // Transition to results screen state
    }
  }
}

void handleInertiaZOscillate(char key) {
  if (key == '#') {
    inertia_yz_screen(); // Go back to Inertia YZ screen
    menuState = 7;
  } else if (key == '1') { // Save the Z oscillation
    z_oscillated = true;
    lcd.clear();
    lcd.print("Z Oscillation Saved");
    delay(1000);
    if (!y_oscillated) { // Check if Y oscillation is pending
      inertia_y_oscillate(); // Redirect to Y oscillate
      menuState = 8;
    } else {
      MMI_final_yz = "678.90"; // Replace with actual calculation
      displayResultsScreen("YZ", MMI_final_xy, MMI_final_yz); // Show results for Inertia YZ
      menuState = 11; // Transition to results screen state
    }
  }
}

void handleResultsScreen(char key, String mode) {
  if (key == '#') {
    if (mode == "XY") {
      inertia_xy_screen(); // Return to Inertia XY screen
      menuState = 6;
    } else if (mode == "YZ") {
      inertia_yz_screen(); // Return to Inertia YZ screen
      menuState = 7;
    }
  } else if (key == '*') {
    lcd.clear();
    lcd.print("Results Saved");
    delay(1000);
    displayMainMenu(); // Go back to the main menu after saving results
    menuState = 0;
  }
}

// Display functions

void displayMainMenu() {
  lcd.clear();
  lcd.setCursor(7, 0);
  lcd.print("Menu");
  lcd.setCursor(0, 1);
  lcd.print("1->Center of Gravity"); // Only show Center of Gravity option
}

void displayCenterOfGravityMenu() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(2, 1);
  lcd.print("1 for xy CoG");
  lcd.setCursor(2, 2);
  lcd.print("2 for zy CoG");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void displayInertiaMenu() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("Inertia");
  lcd.setCursor(0, 1);
  lcd.print("set orientation");
  lcd.setCursor(0, 2);
  lcd.print("1:xy,2:zy");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void cog_screen_2xy() {
  lcd.clear();
  x_COG = "";             // Clear stored input
  decimalEntered = false; // Reset decimal flag

  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(0, 1);
  lcd.print("x coordinate of nose");
  lcd.setCursor(0, 2);
  lcd.print(x_COG);       // Print current input
  lcd.setCursor(18, 2);
  lcd.print("in");
  lcd.setCursor(0, 3);
  lcd.print("Press * after input");
}

void cog_screen_2zy() {
  lcd.clear();
  z_COG = "";             // Clear stored input
  decimalEntered = false; // Reset decimal flag

  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(0, 1);
  lcd.print("z coordinate of nose");
  lcd.setCursor(0, 2);
  lcd.print(z_COG);       // Print current input
  lcd.setCursor(18, 2);
  lcd.print("in");
  lcd.setCursor(0, 3);
  lcd.print("Press * after input");
}

void cog_screen_3_xy() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(0, 1);
  lcd.print("x cg= ");
  lcd.print(Final_x_COG);   // Display the stored x_COG value
  lcd.setCursor(0, 2);
  if (Final_z_COG.length() == 0) {
    lcd.print("1=zy CoG");
  } else {
    lcd.print("1=inertia");
  }
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void cog_screen_3_zy() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(0, 1);
  lcd.print("z cg= ");
  lcd.print(Final_z_COG);   // Display the stored z_COG value
  lcd.setCursor(0, 2);
  if (Final_x_COG.length() == 0) {
    lcd.print("1=xy CoG");
  } else {
    lcd.print("1=inertia");
  }
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void inertia_xy_screen() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("Inertia XY");
  lcd.setCursor(0, 1);
  lcd.print("1: Oscillate X");
  lcd.setCursor(0, 2);
  lcd.print("2: Oscillate Y");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void inertia_yz_screen() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("Inertia YZ");
  lcd.setCursor(0, 1);
  lcd.print("1: Oscillate Y");
  lcd.setCursor(0, 2);
  lcd.print("2: Oscillate Z");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void inertia_x_oscillate() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Inertia in X");
  lcd.setCursor(0, 1);
  
  if(xPeriod > 0){
    lcd.print(xPeriod);
  }
  else{
    lcd.print("Oscillate X");
  }

  lcd.setCursor(0, 2);
  lcd.print("Press 1 to save");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}
void inertia_y_oscillate() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Inertia in Y");
  lcd.setCursor(0, 1);
  lcd.print("Oscillate Y");
  lcd.setCursor(0, 2);
  lcd.print("Press 1 to continue");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}
void inertia_z_oscillate() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Inertia in Z");
  lcd.setCursor(0, 1);
  lcd.print("Oscillate Z");
  lcd.setCursor(0, 2);
  lcd.print("Press 1 to continue");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}


void displayResultsScreen(String mode, String MMI_final_xy, String MMI_final_yz) {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Inertia");

  lcd.setCursor(0, 1);
  if (mode == "XY") {
    lcd.print("Output (XY):");
    lcd.setCursor(0, 2);
    lcd.print(MMI_final_xy); // Print the result value
    lcd.setCursor(13, 2);    // Align "lb in^2" to the right
    lcd.print("lb in^2");
  } else if (mode == "YZ") {
    lcd.print("Output (YZ):");
    lcd.setCursor(0, 2);
    lcd.print(MMI_final_yz); // Print the result value
    lcd.setCursor(13, 2);    // Align "lb in^2" to the right
    lcd.print("lb in^2");
  }

  lcd.setCursor(0, 3);
  lcd.print("#:go back *:save");
}

bool PerformOscillation(){
  float x, y, z;

  if (!isRecording) {
        Serial.println("Recording started!!!");
        isRecording = true;
        startTime = millis(); // Record the starting time
        sample_index = 0; // Reset index for new recording
  }

  if(isRecording){
    if(millis() - startTime < duration * 1000){
      IMU.readGyroscope(x, y, z);
      sensorData[sample_index] = y;
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

      xPeriod = get_period(filtered);
      inertia_x_oscillate();
      
      Serial.print("ok here is the period: ");
      Serial.println(xPeriod);
      Serial.println("\n hoorary!!!");
      
      //Oscillation is done
      oscillationDone = true;
    }
  }

  return false;
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
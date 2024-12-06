#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <string.h> // For strcpy and strncpy
#include <Arduino_LSM6DSOX.h>
#include <HX711.h>
#include <SD.h>

//****************SCREEN VARIABLES*******************

// Initialize the LCD object for a 20x4 display with I2C address 0x27
LiquidCrystal_I2C lcd(0x27, 20, 4);

//************KEYPAD SETUP****************************
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
const int COG_MODE_X = 1;
const int COG_MODE_Z = 2;

String x_cog_coord = ""; // Variable to store user input for x_cog_coord
String z_cog_coord = ""; // Variable to store user input for z_cog_coord
bool decimalEntered = false; // Flag to check if a decimal has been entered
bool x_oscillated = false; // Track if X oscillation is done
bool y_oscillated = false; // Track if Y oscillation is done
bool z_oscillated = false; // Track if Z oscillation is done

//************CENTER OF GRAVITY VARIABLES***********
#define DOUT1 18
#define CLK1 19

#define DOUT2 17
#define CLK2 16

#define DOUT3 25
#define CLK3 24

HX711 scale1;
HX711 scale2;
HX711 scale3;

float calibration_factor1 = 245100.00;
float calibration_factor2 = 228800.00;
float calibration_factor3 = 254700.00;

float platformFrontWeight = 2.290; //brown, scale3 ()
float platformLeftWeight = 2.540; //brown left, scale 2 (23 9/16, )
float platformRightWeight = 2.458; //green, right scale 1 (23 9/16, )

float frontScalePos[2] = {12.5, 7.5};
float leftScalePos[2] = {23.5625, 0.5};
float rightScalePos[2] = {23.5625, 13.5};
float centerOfPlatform[2] = {20.074, 6.976};
float nosePosX = 0;


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

float xPeriod = 0;
float yPeriod = 0;
float zPeriod = 0;

//**************INERTIA EQUATION**************
float platform_period = 0;
float platform_cog_distance = 0;
float uav_cog_distance_x = 0;
float uav_cog_distance_z = 0;

//**************FINAL VALUES**************
float x_cog_final = 0;
float y_cog_final = 0;
float z_cog_final = 0;

float x_mmi_final = 0;
float y_mmi_final = 0;
float z_mmi_final = 0;

//**************SD CARD**************

File outputFile;
const int chipselect = BUILTIN_SDCARD;

void setup() {
  Serial.begin(9600);

  //Initialize Load Cell
  scale1.begin(DOUT1, CLK1);
  scale1.set_gain(64);
  scale1.set_scale();
  scale1.tare(); //Reset the scale to 0
  scale1.set_scale(calibration_factor1);

  scale2.begin(DOUT2, CLK2);
  scale2.set_gain(64);
  scale2.set_scale();
  scale2.tare(); //Reset the scale to 0
  scale2.set_scale(calibration_factor2);

  scale3.begin(DOUT3, CLK3);
  scale3.set_gain(64);
  scale3.set_scale();
  scale3.tare(); //Reset the scale to 0
  scale3.set_scale(calibration_factor3);
  
  lcd.init();         // Initialize the LCD
  lcd.backlight();    // Turn on the backlight
  lcd.clear();        // Clear the screen

  lcd.setCursor(0,0);

 // if (!IMU.begin()) {
 //   Serial.println("Failed to initialize IMU!");
  //  lcd.print("GYRO FAIL");
   // while (1);
  //}

  //if(!SD.begin(chipselect)){
  //  Serial.println("SD initialization failed");
  //  lcd.print("SD CARD FAIL");
  //  while(1);
  //}

  Serial.println("Setup Complete");
  lcd.print("SETUP COMPLETE");
  delay(1000);
  lcd.clear();

  displayMainMenu();  // Display the main menu on startup
}

void loop() {
  char key = customkeypad.getKey(); // Continuously read key press from the keypad
  //int incomingByte;
 // if(Serial.available()){
  //  incomingByte = Serial.read();
  //}
  //else{
 //   incomingByte = -1;
  //}

  if (key != NO_KEY) { // If a key is pressed
    Serial.print(key);
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
      if (cogMode == COG_MODE_X) {
        handleCoGScreen3X(key);
      } else if (cogMode == COG_MODE_Z) {
        handleCoGScreen3Z(key);
      }
    }
    else if (menuState == 5) {
      handleCoGScreen2Z(key); // Handle CoG Screen 2 Z key presses
    }
    else if (menuState == 6) {
      handleInertiaXScreen(key); // Handle Inertia XY screen key presses
    }
    else if (menuState == 7) {
      handleInertiaYScreen(key); // Handle Inertia YZ screen key presses
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
    else if(menuState == 11){
      handleLiveUpdateX(key);
    }
    else if(menuState == 12){
      handleLiveUpdateZ(key);
    }
    else if(menuState == 13){
      handleInertiaZScreen(key);
    }
    else if(menuState == 14){
      handleResultsScreen(key);
    }
  }

  //Inertia Loops
  if(menuState == 8){ //X INERTIA
    if(!oscillationDone){
      PerformOscillation(1);
    }
  }

  if(menuState == 9){ //Y INERTIA
    if(!oscillationDone){
      PerformOscillation(2);
    }
  }

  if(menuState == 10){ //Z INERTIA
    if(!oscillationDone){
      PerformOscillation(3);
    }
  }

  if(menuState == 11){
    float xCG_Diff = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), centerOfPlatform[0], 1);
    live_cg_update(xCG_Diff, 1);
    delay(500);
  }

  if(menuState == 12){
    float zCG_Diff = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), centerOfPlatform[0], 3);
    live_cg_update(zCG_Diff, 2);
    delay(500);
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
    cogMode = COG_MODE_X; // Set mode to xy
    cog_screen_2x(); // Display CoG Screen 2
    menuState = 3;    // Set menuState to CoG Screen 2
  } else if (key == '2') {
    cogMode = COG_MODE_Z; // Set mode to zy
    cog_screen_2z(); // Display CoG Screen 2 Z
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
    displayCenterOfGravityMenu();
    menuState = 1;

    x_oscillated=false;
    y_oscillated=false;
    z_oscillated=false;
  } else if (key == '1') {
    inertia_x_screen(); // Show Inertia XY screen
    menuState = 6;
  } else if (key == '2') {
    inertia_y_screen(); // Show Inertia YZ screen
    menuState = 7;
  } else if (key == '3'){
    inertia_z_screen();
    menuState = 13;
  }
  else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    displayInertiaMenu(); // Redisplay Inertia menu
  }
}

void handleCoGScreen2Menu(char key) {
  if (key == '#') {
    x_cog_coord = "";             // Clear past number
    decimalEntered = false; // Reset decimal flag
    displayCenterOfGravityMenu(); // Go back to Center of Gravity menu
    menuState = 1;          // Set menuState back to Center of Gravity menu
  } else if (key == '*') {
    if (!decimalEntered) {
      if (x_cog_coord.length() < 13) { // Allow space for the decimal and "in"
        x_cog_coord += '.';
        decimalEntered = true;
        lcd.setCursor(0, 2);
        lcd.print(x_cog_coord);
        lcd.setCursor(18, 2);
        lcd.print("in");
      }
    } else {
      lcd.clear();
      lcd.print("Value saved");
      x_cog_final = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), x_cog_coord.toFloat(), 1);
      y_cog_final = CalculateCG(scale3.get_units(),scale2.get_units(), scale1.get_units(), 0,2); //Might need to change this
      delay(1000);
      float xCG_Diff = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), centerOfPlatform[0], 1);
      live_cg_update(xCG_Diff, 1); // Go to the next screen for further actions
      menuState = 11; // Move to the next menu state
    }
  } else if (isdigit(key)) { // Capture numeric input
    if (x_cog_coord.length() < 14) { // Limit input length to fit on the display
      x_cog_coord += key;
      lcd.setCursor(0, 2);
      lcd.print(x_cog_coord);
      lcd.setCursor(18, 2);
      lcd.print("in");
    }
  }
}

void handleCoGScreen2Z(char key) {
  if (key == '#') {
    z_cog_coord = "";             // Clear past number
    decimalEntered = false; // Reset decimal flag
    displayCenterOfGravityMenu(); // Go back to Center of Gravity menu
    menuState = 1;          // Set menuState back to Center of Gravity menu
  } else if (key == '*') {
    if (!decimalEntered) {
      if (z_cog_coord.length() < 13) { // Allow space for the decimal and "in"
        z_cog_coord += '.';
        decimalEntered = true;
        lcd.setCursor(0, 2);
        lcd.print(z_cog_coord);
        lcd.setCursor(18, 2);
        lcd.print("in");
      }
    } else {
      lcd.clear();
      lcd.print("Value saved");
      z_cog_final = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), z_cog_coord.toFloat(), 3);
      delay(1000);
      float zCG_Diff = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), centerOfPlatform[0],3);
      live_cg_update(zCG_Diff,2); // Go to the next screen
      menuState = 12; // Move to the next menu state
    }
  } else if (isdigit(key)) { // Capture numeric input
    if (z_cog_coord.length() < 14) { // Limit input length to fit on the display
      z_cog_coord += key;
      lcd.setCursor(0, 2);
      lcd.print(z_cog_coord);
      lcd.setCursor(18, 2);
      lcd.print("in");
    }
  }
}

void handleCoGScreen3X(char key) {
  if (key == '#') {
    cog_screen_2x(); // Go back to previous screen
    menuState = 3;
  } else if (key == '1') {
    if (z_cog_coord.length() == 0) { // Check if z_COG has been saved
      cog_screen_2z(); // Redirect to z_COG input if missing
      menuState = 5;
    } else {
      displayInertiaMenu(); // Access inertia menu if both values are saved
      menuState = 2;
    }
  } else if (key == '*') {
    lcd.clear();
    lcd.print("Value saved");
    delay(1000);
    cog_screen_3x(); // Stay on the same screen
  }
}

void handleCoGScreen3Z(char key) {
  if (key == '#') {
    cog_screen_2z(); // Go back to previous screen
    menuState = 5;
  } else if (key == '1') {
    if (x_cog_coord.length() == 0) { // Check if x_COG has been saved
      cog_screen_2x(); // Redirect to x_COG input if missing
      menuState = 3;
    } else {
      displayInertiaMenu(); // Access inertia menu if both values are saved
      menuState = 2;
    }
  } else if (key == '*') {
    lcd.clear();
    lcd.print("Value saved");
    delay(1000);
    cog_screen_3z(); // Stay on the same screen
  }
}

void handleLiveUpdateX(char key){
  if(key=='#'){
    cog_screen_2x(); // Go back to previous screen
    menuState = 3;
  }
  else if(key=='1'){
    //Save value
    cog_screen_3x();
    menuState = 4;
  }
}

void handleLiveUpdateZ(char key){
  if(key=='#'){
    cog_screen_2z(); // Go back to previous screen
    menuState = 3;
  }
  else if(key=='1'){
    //Save value
    cog_screen_3z();
    menuState = 4;
  }
}

void handleInertiaXScreen(char key) {
  if (key == '#') {
    displayInertiaMenu(); // Go back to inertia menu
    menuState = 2;
  } else if (key == '1') { // Handle X oscillation
    inertia_x_oscillate(); // Go to the X oscillation screen
    menuState = 8; // Update menu state for X oscillation
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    inertia_x_screen(); // Redisplay the Inertia XY screen
  }
}

void handleInertiaYScreen(char key) {
  if (key == '#') {
    displayInertiaMenu(); // Go back to inertia menu
    menuState = 2;
  } else if (key == '1') { // Handle Y oscillation
    inertia_y_oscillate(); // Go to the Y oscillation screen
    menuState = 9; // Update menu state for Y oscillation
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    inertia_y_screen(); // Redisplay the Inertia YZ screen
  }
}

void handleInertiaZScreen(char key) {
  if (key == '#') {
    displayInertiaMenu(); // Go back to inertia menu
    menuState = 2;
  } else if (key == '1') { // Handle Y oscillation
    inertia_z_oscillate(); // Go to the Y oscillation screen
    menuState = 10; // Update menu state for Y oscillation
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    inertia_z_screen(); // Redisplay the Inertia YZ screen
  }
}

void handleInertiaXOscillate(char key) {
  if (key == '#') {
    inertia_x_screen(); // Go back to Inertia XY screen
    oscillationDone = false;
    menuState = 6;
  } else if (key == '1') { // Save the X oscillation
    x_oscillated = true;
    x_mmi_final = CalculateMMI(xPeriod, 0);
    lcd.clear();
    lcd.print("X Oscillation Saved");
    delay(1000);
    if (!y_oscillated || !z_oscillated) { // Check if Y oscillation is pending
      displayInertiaMenu(); // Redirect to Y oscillate
      menuState = 2;
    } else {
      displayResultsScreen(); // Show results for Inertia XY
      menuState = 14; // Transition to results screen state
    }
    oscillationDone = false;
  }
}

void handleInertiaYOscillate(char key) {
  if (key == '#') {
    inertia_y_screen(); // Go back to Inertia XY screen
    menuState = 6;
  } else if (key == '1') { // Save the Y oscillation
    y_oscillated = true;
    y_mmi_final = CalculateMMI(yPeriod, 0);
    lcd.clear();
    lcd.print("Y Oscillation Saved");
    delay(1000);
    if (!x_oscillated || !z_oscillated) { // Check if X oscillation is pending
      displayInertiaMenu(); // Redirect to X oscillate
      menuState = 2;
    } else {
      displayResultsScreen(); // Show results for Inertia XY
      menuState = 14; // Transition to results screen state
    }
    oscillationDone = false;
  }
}

void handleInertiaZOscillate(char key) {
  if (key == '#') {
    inertia_z_screen(); // Go back to Inertia YZ screen
    menuState = 13;
  } else if (key == '1') { // Save the Z oscillation
    z_oscillated = true;
    z_mmi_final = CalculateMMI(zPeriod, 0);
    lcd.clear();
    lcd.print("Z Oscillation Saved");
    delay(1000);
    if (!y_oscillated || !x_oscillated) { // Check if Y oscillation is pending
      displayInertiaMenu();// Redirect to Y oscillate
      menuState = 2;
    } else {
      displayResultsScreen(); // Show results for Inertia YZ
      menuState = 14; // Transition to results screen state
    }
    oscillationDone = false;
  }
}

void handleResultsScreen(char key) {
  if (key == '#') {
    x_oscillated = false;
    y_oscillated = false;
    z_oscillated = false;
    menuState = 2;
    displayInertiaMenu();

  } else if (key == '*') {
    SaveToSDCard(); //Save results to SD Card for MMI and COG
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
  lcd.print("1:Center of Gravity"); // Only show Center of Gravity option
}

void displayCenterOfGravityMenu() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(2, 1);
  lcd.print("1: X CoG");
  lcd.setCursor(2, 2);
  lcd.print("2: Z CoG");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void displayInertiaMenu() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("Inertia");
  lcd.setCursor(0, 1);
  lcd.print("Set Orientation");
  lcd.setCursor(0, 2);

  if(!x_oscillated){
    lcd.print("1:X ");
  }
  if(!y_oscillated){
    lcd.print("2:Y ");
  }
  if(!z_oscillated){
    lcd.print("3:Z ");
  }

  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void cog_screen_2x() {
  lcd.clear();
  x_cog_coord = "";             // Clear stored input
  decimalEntered = false; // Reset decimal flag

  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(0, 1);
  lcd.print("X Coordinate of Nose");
  lcd.setCursor(0, 2);
  lcd.print(x_cog_coord);       // Print current input
  lcd.setCursor(18, 2);
  lcd.print("in");
  lcd.setCursor(0, 3);
  lcd.print("Press * after input");
}

void cog_screen_2z() {
  lcd.clear();
  z_cog_coord = "";             // Clear stored input
  decimalEntered = false; // Reset decimal flag

  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(0, 1);
  lcd.print("Z Coordinate of Nose");
  lcd.setCursor(0, 2);
  lcd.print(z_cog_coord);       // Print current input
  lcd.setCursor(18, 2);
  lcd.print("in");
  lcd.setCursor(0, 3);
  lcd.print("Press * after input");
}

void cog_screen_3x() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(0, 1);
  lcd.print("x cg= ");
  lcd.print(x_cog_coord);   // Display the stored x_COG value
  lcd.setCursor(0, 2);
  if (z_cog_coord.length() == 0) {
    lcd.print("1:Z CoG");
  } else {
    lcd.print("1:Inertia");
  }
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void cog_screen_3z() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(0, 1);
  lcd.print("z cg= ");
  lcd.print(z_cog_coord);   // Display the stored z_COG value
  lcd.setCursor(0, 2);
  if (x_cog_coord.length() == 0) {
    lcd.print("1:X CoG");
  } else {
    lcd.print("1:Inertia");
  }
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

//axis:
//1 -> x
//2 -> z
void live_cg_update(float differenceVal, int axis){
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Center of Gravity");
  lcd.setCursor(0,1);
  if(axis == 1){
    lcd.print("Move x: ");
  }
  else{
    lcd.print("Move z: ");
  }
  lcd.print(differenceVal,2);
  lcd.setCursor(0, 2);
  lcd.print("Press 1 when done");
  lcd.setCursor(0,3);
  lcd.print("Press # to go back");

}

void inertia_x_screen() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("Inertia X");
  lcd.setCursor(0, 1);
  lcd.print("1: Start Oscillate X");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void inertia_y_screen() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("Inertia Y");
  lcd.setCursor(0, 1);
  lcd.print("1: Start Oscillate Y");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void inertia_z_screen() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("Inertia Z");
  lcd.setCursor(0, 1);
  lcd.print("1: Oscillate Z");
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
    lcd.print("CALCULATING");
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
  
  if(yPeriod > 0){
    lcd.print(yPeriod);
  }
  else{
    lcd.print("CALCULATING");
  }

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
  
  if(zPeriod > 0){
    lcd.print(zPeriod);
  }
  else{
    lcd.print("CALCULATING");
  }

  lcd.setCursor(0, 2);
  lcd.print("Press 1 to continue");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}


void displayResultsScreen() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Inertia");

  //Print format and units
  lcd.setCursor(0, 1);
  lcd.print("Form=X,Y,Z lb in^2");
  
  //Print in proper format
  lcd.setCursor(0, 2);
  lcd.print(x_mmi_final, 2); // Print the result value
  lcd.print(",");
  lcd.print(y_mmi_final, 2);
  lcd.print(",");
  lcd.print(z_mmi_final, 2);

  lcd.setCursor(0, 3);
  lcd.print("#:Go Back *:Save All");
}

//Calculate Period
//int direction:
//  1: x-axis oscillation
//  2: y-axis oscillation
//  3: z-axis oscillation
bool PerformOscillation(int direction){
  //Initialize gyroscope axis readings
  float x, y, z;

  //Initialize Reading Loop
  if (!isRecording) {
    Serial.println("Recording started!!!");
    isRecording = true;
    startTime = millis(); // Record the starting time
    sample_index = 0; // Reset index for new recording
  }

  //Data Collection and Period Calculation
  if(isRecording){
    if(millis() - startTime < duration * 1000){ //Collect data from 
      IMU.readGyroscope(x, y, z); //get values from readGyroscope

      float data = 0;
      if(direction == 1){
        data = x;
      }
      else if(direction == 2){
        data = x;
      }
      else if(direction == 3){
        data = y;
      }

      //Save data and time for period calculation
      sensorData[sample_index] = data;
      timeData[sample_index] = millis();
      sample_index++;

      //Fix sample rate
      delay(1000/sampleRate);
    }
    else{
      isRecording = false; //finish oscillation

      //Send data through averaging
      float filtered[arraySize];
      for (int i = 0; i < arraySize; i++) {
        filtered[i] = sensorData[i];
      }
      moving_avg_filter(sensorData, 5, filtered);

      //Get period and and print to screen
      if(direction == 1){
        xPeriod = get_period(filtered);
        inertia_x_oscillate();
      }
      else if(direction == 2){
        yPeriod = 1674;//get_period(filtered);
        inertia_y_oscillate();
      }
      else if(direction == 3){
        zPeriod = get_period(filtered);
        inertia_z_oscillate();
      }
      
      //Debug Prints
      Serial.print("ok here is the period: ");
      Serial.println(xPeriod);
      Serial.println("\n hoorary!!!");
      
      //Oscillation is done
      oscillationDone = true;
    }
  }

  //Oscillation is not done
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

float CalculateMMI(float period, float cog_distance){
  return 0;
}

float CalculateCG(float frontWeight, float leftWeight, float rightWeight, float referencePoint, int dir){

    //total weight of the model - total weight of the platform : calibration
    float modelWeight = (frontWeight+leftWeight+rightWeight)-(platformFrontWeight+platformLeftWeight+platformRightWeight);
    float cg = 0;

    if(dir == 1){ //x
      cg = ((frontWeight-platformFrontWeight)*frontScalePos[0]+(leftWeight-platformLeftWeight)*leftScalePos[0]+(rightWeight-platformRightWeight)*rightScalePos[0])/(modelWeight);
    }
    else if(dir == 2){ //y
      cg = ((frontWeight-platformFrontWeight)*frontScalePos[1]+(leftWeight-platformLeftWeight)*leftScalePos[1]+(rightWeight-platformRightWeight)*rightScalePos[1])/(modelWeight);
    }
    else if(dir == 3){ //z
      cg = ((frontWeight-platformFrontWeight)*frontScalePos[0]+(leftWeight-platformLeftWeight)*leftScalePos[0]+(rightWeight-platformRightWeight)*rightScalePos[0])/(modelWeight);
    }

    return cg - referencePoint;

}

void SaveToSDCard(){
  outputFile = SD.open("output.txt", FILE_WRITE);

  if(outputFile){
    //PRINT COG
    outputFile.print(x_cog_final);
    outputFile.print(",");
    outputFile.print(y_cog_final);
    outputFile.print(",");
    outputFile.print(z_cog_final);

    outputFile.println();

    //PRINT MMI
    outputFile.print(x_mmi_final);
    outputFile.print(",");
    outputFile.print(y_mmi_final);
    outputFile.print(",");
    outputFile.print(z_mmi_final);

  }

  outputFile.close();
}
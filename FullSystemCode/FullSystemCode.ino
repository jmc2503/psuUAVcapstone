#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <string.h>  // For strcpy and strncpy
#include <Adafruit_LSM6DSOX.h>
#include "HX711.h"
#include <SD.h>

//****************SENSOR VARIABLES*******************

// Initialize the LCD object for a 20x4 display with I2C address 0x27
LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_LSM6DSOX sox;

//************KEYPAD SETUP****************************
const byte ROWS = 4;  // Four rows
const byte COLS = 3;  // Three columns (1-9, *, and #)
char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
byte rowPins[ROWS] = { 3, 8, 7, 5 };  // Connect keypad ROW1, ROW2, ROW3, ROW4 to Teensy pins
byte colPins[COLS] = { 4, 2, 6 };     // Connect keypad COL1, COL2, COL3 to Teensy pins
Keypad customkeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int menuState = 0;  // 0: Main menu, 1: Center of Gravity, 2: Inertia, 3: CoG Screen 2, 4: Next CoG Screen, 5: CoG Screen 2 Z
int cogMode = 0;    // 0: none, 1: xy, 2: zy
int resultChoice = 0;
const int COG_MODE_X = 1;
const int COG_MODE_Z = 2;

String x_cog_coord = "";      // Variable to store user input for x_cog_coord
String z_cog_coord = "";      // Variable to store user input for z_cog_coord
bool decimalEntered = false;  // Flag to check if a decimal has been entered
bool x_oscillated = false;    // Track if X oscillation is done
bool y_oscillated = false;    // Track if Y oscillation is done
bool z_oscillated = false;    // Track if Z oscillation is done

//************CENTER OF GRAVITY VARIABLES***********
#define DOUT1 21 
#define CLK1 22

#define DOUT2 17
#define CLK2 16

#define DOUT3 25
#define CLK3 24

HX711 scale1;
HX711 scale2;
HX711 scale3;

float calibration_factor1 = 245100.00;  //Calibration factor for scale 1
float calibration_factor2 = 228800.00;  //Calibration factor for scale 2
float calibration_factor3 = 254700.00;  //Calibration factor for scale 3

float platform_front_weight = 2.222;  //brown, scale3 ()
float platform_left_weight = 2.175;   //brown left, scale 2 (23 9/16, )
float platform_right_weight = 2.248;  //green, right scale 1 (23 9/16, )

float front_scale_pos[2] = { 12.5, 7 };
float left_scale_pos[2] = { 24, 0.5 };
float right_scale_pos[2] = { 24, 13.5 };
float cg_platform[2] = { 20.108, 7.045 };

//************INERTIA VARIABLES**************

const int duration = 10;  // How long the test runs for
const int sampleRate = 100;
const int sampleTime = 1000 / sampleRate;
const int arraySize = duration * sampleRate;

float sensor_data[arraySize];        // Readings from the gyroscope
unsigned long time_data[arraySize];  // What time those readings occur for calculating period
int sample_index = 0;
bool isRecording = false;
unsigned long startTime;

float x_period = 0;  //Period calculated from gyro readings in X
float y_period = 0;  //Period calculated from gyro readings in Y
float z_period = 0;  //Period calculated from gyro readings in Z

//**************INERTIA EQUATION**************
#define g 386.08858267717          //gravity in in/s^2
float platform_period_xz = 1.562;  //Period of platform with nothing on it
float platform_period_y = 1.566;

float platform_top_distance_xz = 25.5;   //Distance from top of platform to rotation point for xz intertia
float platform_top_distance_y = 17.875;  //Distance from top of platform to rotation point for y inertia

float platform_cog_distance_xz = 26;  //Distance from platform cog to rotation point in
float platform_cog_distance_y = 18.375;

float platform_weight = 0;     //Platform weigth in lbs
float uav_cog_distance_x = 0;  //Calculated distance from uav cog to rotation point in x orientation
float uav_cog_distance_y = 0;  //Calculated distance from uav cog to rotation point in y direction
float uav_cog_distance_z = 0;  //Calculated distance from uav cog to rotation point in z orientation
float model_weight_final = 0;  //Calculated model weight by load cells during CoG calculation

//**************FINAL VALUES**************
float x_cog_final = 0;
float y_cog_final = 0;
float z_cog_final = 0;

float x_mmi_final = 0;
float y_mmi_final = 0;
float z_mmi_final = 0;

//**************SD CARD**************

File output_file;
const int chipselect = BUILTIN_SDCARD;

void setup() {
  Serial.begin(9600);

  //Initialize Load Cell
  scale1.begin(DOUT1, CLK1);
  scale1.set_gain(64);
  scale1.set_scale(calibration_factor1);
  //scale1.set_scale();
  scale1.tare();  //Reset the scale to 0


  scale2.begin(DOUT2, CLK2);
  scale2.set_gain(64);
  scale2.set_scale(calibration_factor2);
  //scale2.set_scale();
  scale2.tare();  //Reset the scale to 0

  scale3.begin(DOUT3, CLK3);
  scale3.set_gain(64);
  scale3.set_scale(calibration_factor3);
  scale3.tare();  //Reset the scale to 0

  lcd.init();       // Initialize the LCD
  lcd.backlight();  // Turn on the backlight
  lcd.clear();      // Clear the screen

  lcd.setCursor(0, 0);

  //Check that gyroscope is connected
  if (!sox.begin_I2C()) {
    Serial.println("Failed to find LSM6DSOX chip");
    lcd.print("GYRO FAIL");
    while (1);
  }

  //Check that the SD card is available
  if (!SD.begin(chipselect)) {
    Serial.println("SD initialization failed");
    lcd.print("SD CARD FAIL");
    while (1);
  }

  sox.setGyroDataRate(LSM6DS_RATE_3_33K_HZ);

  platform_weight = platform_front_weight + platform_right_weight + platform_left_weight;

  Serial.println("Setup Complete");
  lcd.print("SETUP COMPLETE");
  delay(1000);
  lcd.clear();

  displayMainMenu();  // Display the main menu on startup
}

void loop() {
  char key = customkeypad.getKey();  // Continuously read key press from the keypad

  if (key != NO_KEY) {  // If a key is pressed
    if (menuState == 0) {
      handleMainMenu(key);  // Handle main menu key presses
    } else if (menuState == 1) {
      handleCenterOfGravityMenu(key);  // Handle Center of Gravity menu key presses
    } else if (menuState == 2) {
      handleInertiaMenu(key);  // Handle Inertia menu key presses
    } else if (menuState == 3) {
      handleCoGScreen2Menu(key);  // Handle CoG Screen 2 key presses
    } else if (menuState == 4) {
      if (cogMode == COG_MODE_X) {
        handleCoGScreen3X(key);
      } else if (cogMode == COG_MODE_Z) {
        handleCoGScreen3Z(key);
      }
    } else if (menuState == 5) {
      handleCoGScreen2Z(key);  // Handle CoG Screen 2 Z key presses
    } else if (menuState == 6) {
      handleInertiaXScreen(key);  // Handle Inertia X screen key presses
    } else if (menuState == 7) {
      handleInertiaYScreen(key);  // Handle Inertia Y screen key presses
    } else if (menuState == 8) {
      handleInertiaXOscillate(key);  // Handle X oscillation key presses
    } else if (menuState == 9) {
      handleInertiaYOscillate(key);  // Handle Y oscillation key presses
    } else if (menuState == 10) {
      handleInertiaZOscillate(key);  // Handle Z oscillation key presses
    } else if (menuState == 11) {    //Handle live CoG update for X
      handleLiveUpdateX(key);
    } else if (menuState == 12) {  //Handle live CoG update for Z
      handleLiveUpdateZ(key);
    } else if (menuState == 13) {  //Handle Inertia Z screen key presses
      handleInertiaZScreen(key);
    } else if (menuState == 14) {  //Handle results screen key presses
      handleResultsScreen(key);
    }
  }

  //Inertia Loops
  if (menuState == 8) {  //X INERTIA
    if (!x_oscillated) {
      PerformOscillation(1);  //
    }
  }

  if (menuState == 9) {  //Y INERTIA
    if (!y_oscillated) {
      PerformOscillation(2);
    }
  }

  if (menuState == 10) {  //Z INERTIA
    if (!z_oscillated) {
      PerformOscillation(3);
    }
  }

  if (menuState == 11) {  //Live calculate the distance the structure should be moved to align CoG in X
    float xCG_Diff = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), cg_platform[0], 1);
    live_cg_update(xCG_Diff, 1);  //Write new value to the screen
    delay(500);                   //Delay to make screen update slower
  }

  if (menuState == 12) {  //Live calculate the distance the structure should be moved to align CoG in Z
    float zCG_Diff = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), cg_platform[0], 3);
    live_cg_update(zCG_Diff, 2);  //Write new value to the screen
    delay(500);                   //Delay to make screen update slower
  }

}


void handleMainMenu(char key) {
  if (key == '1') {
    displayCenterOfGravityMenu();
    menuState = 1;  // Set menuState to Center of Gravity menu
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    displayMainMenu();  // Redisplay the main menu
  }
}

void handleCenterOfGravityMenu(char key) {
  if (key == '#') {
    displayMainMenu();  // Go back to main menu
    menuState = 0;      // Reset to main menu state
  } else if (key == '1') {
    cogMode = COG_MODE_X;  // Set mode to xy
    cog_screen_2x();       // Display CoG Screen 2
    menuState = 3;         // Set menuState to CoG Screen 2
  } else if (key == '2') {
    cogMode = COG_MODE_Z;  // Set mode to zy
    cog_screen_2z();       // Display CoG Screen 2 Z
    menuState = 5;         // Set menuState to CoG Screen 2 Z
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    displayCenterOfGravityMenu();  // Redisplay Center of Gravity menu
  }
}

void handleInertiaMenu(char key) {
  if (key == '#') {
    displayCenterOfGravityMenu();
    menuState = 1;

    //Reset oscillations
    x_oscillated = false;
    y_oscillated = false;
    z_oscillated = false;
  } else if (key == '1') {
    inertia_x_screen();  // Show Inertia X screen
    menuState = 6;
  } else if (key == '2') {
    inertia_y_screen();  // Show Inertia Y screen
    menuState = 7;
  } else if (key == '3') {
    inertia_z_screen();  // Show Inertia Z screen
    menuState = 13;
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    displayInertiaMenu();  // Redisplay Inertia menu
  }
}

void handleCoGScreen2Menu(char key) {
  if (key == '#') {
    x_cog_coord = "";              // Clear past number
    decimalEntered = false;        // Reset decimal flag
    displayCenterOfGravityMenu();  // Go back to Center of Gravity menu
    menuState = 1;                 // Set menuState back to Center of Gravity menu
  } else if (key == '*') {
    if (!decimalEntered) {
      if (x_cog_coord.length() < 13) {  // Allow space for the decimal and "in"
        x_cog_coord += '.';
        decimalEntered = true;
        lcd.setCursor(0, 2);
        lcd.print(x_cog_coord);
        lcd.setCursor(18, 2);
        lcd.print("in");
      }
    } else {
      lcd.clear();
      lcd.print("Value saved");                                                                                         //Nose position stored in x_cog_coord
      x_cog_final = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), x_cog_coord.toFloat(), 1);  //CoG of UAV relative to the position entered in X
      y_cog_final = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), 7, 2);                      //CoG of UAV relative to the center axis in Y
      uav_cog_distance_z = platform_top_distance_xz - x_cog_final;
      model_weight_final = GetFinalModelWeight(scale3.get_units(), scale2.get_units(), scale1.get_units());
      delay(1000);

      float xCG_Diff = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), cg_platform[0], 1);  //For live update
      live_cg_update(xCG_Diff, 1);                                                                                  // Go to the next screen for further actions
      menuState = 11;                                                                                               // Move to the next menu state
    }
  } else if (isdigit(key)) {          // Capture numeric input
    if (x_cog_coord.length() < 14) {  // Limit input length to fit on the display
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
    z_cog_coord = "";              // Clear past number
    decimalEntered = false;        // Reset decimal flag
    displayCenterOfGravityMenu();  // Go back to Center of Gravity menu
    menuState = 1;                 // Set menuState back to Center of Gravity menu
  } else if (key == '*') {
    if (!decimalEntered) {
      if (z_cog_coord.length() < 13) {  // Allow space for the decimal and "in"
        z_cog_coord += '.';
        decimalEntered = true;
        lcd.setCursor(0, 2);
        lcd.print(z_cog_coord);
        lcd.setCursor(18, 2);
        lcd.print("in");
      }
    } else {
      lcd.clear();
      lcd.print("Value saved");                                                                                         //Nose position in Z stored in z_cog_coord
      z_cog_final = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), z_cog_coord.toFloat(), 3);  //CoG of UAV relative to the position entered in Z
      uav_cog_distance_x = platform_top_distance_xz - z_cog_final;
      uav_cog_distance_y = platform_top_distance_y - z_cog_final;
      delay(1000);

      float zCG_Diff = CalculateCG(scale3.get_units(), scale2.get_units(), scale1.get_units(), cg_platform[0], 3);
      live_cg_update(zCG_Diff, 2);  // Go to the next screen
      menuState = 12;               // Move to the next menu state
    }
  } else if (isdigit(key)) {          // Capture numeric input
    if (z_cog_coord.length() < 14) {  // Limit input length to fit on the display
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
    cog_screen_2x();  // Go back to previous screen
    menuState = 3;
  } else if (key == '1') {
    if (z_cog_coord.length() == 0) {  // Check if z_COG has been saved
      cog_screen_2z();                // Redirect to z_COG input if missing
      menuState = 5;
    } else {
      displayInertiaMenu();  // Access inertia menu if both values are saved
      menuState = 2;
    }
  } else if (key == '*') {
    lcd.clear();
    lcd.print("Value saved");
    delay(1000);
    cog_screen_3x();  // Stay on the same screen
  }
}

void handleCoGScreen3Z(char key) {
  if (key == '#') {
    cog_screen_2z();  // Go back to previous screen
    menuState = 5;
  } else if (key == '1') {
    if (x_cog_coord.length() == 0) {  // Check if x_COG has been saved
      cog_screen_2x();                // Redirect to x_COG input if missing
      menuState = 3;
    } else {
      displayInertiaMenu();  // Access inertia menu if both values are saved
      menuState = 2;
    }
  } else if (key == '*') {
    lcd.clear();
    lcd.print("Value saved");
    delay(1000);
    cog_screen_3z();  // Stay on the same screen
  }
}

void handleLiveUpdateX(char key) {
  if (key == '#') {
    cog_screen_2x();  // Go back to previous screen
    menuState = 3;
  } else if (key == '1') {
    //Save value
    cog_screen_3x();
    menuState = 4;
  }
}

void handleLiveUpdateZ(char key) {
  if (key == '#') {
    cog_screen_2z();  // Go back to previous screen
    menuState = 3;
  } else if (key == '1') {
    //Save value
    cog_screen_3z();
    menuState = 4;
  }
}

void handleInertiaXScreen(char key) {
  if (key == '#') {
    displayInertiaMenu();  // Go back to inertia menu
    menuState = 2;
  } else if (key == '1') {  // Handle X oscillation
    inertia_x_oscillate();  // Go to the X oscillation screen
    menuState = 8;          // Update menu state for X oscillation
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    inertia_x_screen();  // Redisplay the Inertia X screen
  }
}

void handleInertiaYScreen(char key) {
  if (key == '#') {
    displayInertiaMenu();  // Go back to inertia menu
    menuState = 2;
  } else if (key == '1') {  // Handle Y oscillation
    inertia_y_oscillate();  // Go to the Y oscillation screen
    menuState = 9;          // Update menu state for Y oscillation
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    inertia_y_screen();  // Redisplay the Inertia Y screen
  }
}

void handleInertiaZScreen(char key) {
  if (key == '#') {
    displayInertiaMenu();  // Go back to inertia menu
    menuState = 2;
  } else if (key == '1') {  // Handle Y oscillation
    inertia_z_oscillate();  // Go to the Y oscillation screen
    menuState = 10;         // Update menu state for Y oscillation
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(1000);
    inertia_z_screen();  // Redisplay the Inertia Y screen
  }
}

void handleInertiaXOscillate(char key) {
  if (key == '#') {
    inertia_x_screen();    // Go back to Inertia X screen
    x_oscillated = false;  //reset oscillation
    menuState = 6;
  } else if (key == '1') {  // Save the X oscillation
    x_mmi_final = CalculateMMI(x_period, uav_cog_distance_x, 1);
    lcd.clear();
    lcd.print("X Oscillation Saved");
    delay(1000);
    if (!y_oscillated || !z_oscillated) {  // Check if Y or Z oscillation is pending
      displayInertiaMenu();                // Redirect to Y oscillate
      menuState = 2;
    } else {
      displayResultsScreen(resultChoice);  // Show results for Inertia
      menuState = 14;                      // Transition to results screen state
    }
  }
}

void handleInertiaYOscillate(char key) {
  if (key == '#') {
    inertia_y_screen();    // Go back to Inertia Y screen
    y_oscillated = false;  //reset oscillation
    menuState = 7;
  } else if (key == '1') {  // Save the Y oscillation
    y_mmi_final = CalculateMMI(y_period, uav_cog_distance_y, 2);
    lcd.clear();
    lcd.print("Y Oscillation Saved");
    delay(1000);
    if (!x_oscillated || !z_oscillated) {  // Check if X or Z oscillation is pending
      displayInertiaMenu();                // Redirect to X oscillate
      menuState = 2;
    } else {
      displayResultsScreen(resultChoice);  // Show results for Inertia
      menuState = 14;                      // Transition to results screen state
    }
  }
}

void handleInertiaZOscillate(char key) {
  if (key == '#') {
    inertia_z_screen();    // Go back to Inertia YZ screen
    z_oscillated = false;  //reset oscillation
    menuState = 13;
  } else if (key == '1') {  // Save the Z oscillation
    z_mmi_final = CalculateMMI(z_period, uav_cog_distance_z, 3);
    lcd.clear();
    lcd.print("Z Oscillation Saved");
    delay(1000);
    if (!y_oscillated || !x_oscillated) {  // Check if Y or Xoscillation is pending
      displayInertiaMenu();                // Redirect to Y oscillate
      menuState = 2;
    } else {
      displayResultsScreen(resultChoice);  // Show results for Inertia YZ
      menuState = 14;                      // Transition to results screen state
    }
  }
}

void handleResultsScreen(char key) {
  if (key == '#') {  //return to oscillation screen
    //Reset oscillation variables to run them again
    x_oscillated = false;
    y_oscillated = false;
    z_oscillated = false;
    resultChoice = 0;
    menuState = 2;
    displayInertiaMenu();

  } else if (key == '1') {
    resultChoice++;
    resultChoice = resultChoice % 6;
    displayResultsScreen(resultChoice);
  } else if (key == '*') {
    SaveToSDCard();  //Save results to SD Card for MMI and COG
    lcd.clear();
    lcd.print("Results Saved");
    delay(1000);
    displayMainMenu();  // Go back to the main menu after saving results
    menuState = 0;

    x_oscillated = false;
    y_oscillated = false;
    z_oscillated = false;
    x_cog_coord = "";
    z_cog_coord = "";
    resultChoice = 0;
  }
}

// Display functions

void displayMainMenu() {
  lcd.clear();
  lcd.setCursor(7, 0);
  lcd.print("Menu");
  lcd.setCursor(0, 1);
  lcd.print("1:Center of Gravity");  // Only show Center of Gravity option
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

  if (!x_oscillated) {
    lcd.print("1:X ");
  }
  if (!y_oscillated) {
    lcd.print("2:Y ");
  }
  if (!z_oscillated) {
    lcd.print("3:Z ");
  }

  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}

void cog_screen_2x() {
  lcd.clear();
  x_cog_coord = "";        // Clear stored input
  decimalEntered = false;  // Reset decimal flag

  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(0, 1);
  lcd.print("X Coordinate of Nose");
  lcd.setCursor(0, 2);
  lcd.print(x_cog_coord);  // Print current input
  lcd.setCursor(18, 2);
  lcd.print("in");
  lcd.setCursor(0, 3);
  lcd.print("Press * after input");
}

void cog_screen_2z() {
  lcd.clear();
  z_cog_coord = "";        // Clear stored input
  decimalEntered = false;  // Reset decimal flag

  lcd.setCursor(1, 0);
  lcd.print("Center Of Gravity");
  lcd.setCursor(0, 1);
  lcd.print("Z Coordinate of Nose");
  lcd.setCursor(0, 2);
  lcd.print(z_cog_coord);  // Print current input
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
  lcd.print(x_cog_coord);  // Display the stored x_COG value
  lcd.setCursor(0, 2);
  if (z_cog_coord.length() == 0) {  //Do Z CoG if not done, otherwise go to inertia
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
  lcd.print(z_cog_coord);  // Display the stored z_COG value
  lcd.setCursor(0, 2);
  if (x_cog_coord.length() == 0) {  //Do X CoG if not done, otherwise go to inertia
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
void live_cg_update(float differenceVal, int axis) {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Center of Gravity");
  lcd.setCursor(0, 1);
  if (axis == 1) {  //Choose between displaying x or z
    lcd.print("Move x: ");
  } else {
    lcd.print("Move z: ");
  }
  lcd.print(differenceVal, 2);
  lcd.setCursor(0, 2);
  lcd.print("Press 1 when done");
  lcd.setCursor(0, 3);
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

  if (x_oscillated) {  //Display period found once calculated
    lcd.print(x_period);
  } else {
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

  if (y_oscillated) {  //Display period found once calculated
    lcd.print(y_period);
  } else {
    lcd.print("CALCULATING");
  }

  lcd.setCursor(0, 2);
  lcd.print("Press 1 to save");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}
void inertia_z_oscillate() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Inertia in Z");
  lcd.setCursor(0, 1);

  if (z_oscillated) {  //Display period found once calculated
    lcd.print(z_period);
  } else {
    lcd.print("CALCULATING");
  }

  lcd.setCursor(0, 2);
  lcd.print("Press 1 to save");
  lcd.setCursor(0, 3);
  lcd.print("Press # to go back");
}


void displayResultsScreen(int choice) {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Results");

  //Print format and units
  lcd.setCursor(0, 1);
  switch (choice) {
    case 0:
      lcd.print("X mmi:");
      lcd.print(x_mmi_final, 1);
      break;
    case 1:
      lcd.print("Y mmi:");
      lcd.print(y_mmi_final, 1);
      break;
    case 2:
      lcd.print("Z mmi:");
      lcd.print(z_mmi_final, 1);
      break;
    case 3:
      lcd.print("X cg:");
      lcd.print(x_cog_final);
      break;
    case 4:
      lcd.print("Y cg:");
      lcd.print(y_cog_final);
      break;
    case 5:
      lcd.print("Z cg:");
      lcd.print(z_cog_final);
      break;
  }

  //Print in proper format
  lcd.setCursor(0, 2);
  lcd.print("1: Next Result");

  lcd.setCursor(0, 3);
  lcd.print("#:Go Back *:Save All");
}

//**************INERTIA AND CG FUNCTIONS****************

//Calculate Period by sampling gyroscope
//int direction:
//  1: x-axis oscillation
//  2: y-axis oscillation
//  3: z-axis oscillation
void PerformOscillation(int direction) {
  //Initialize gyroscope axis readings
  sensors_event_t gyro;

  //These are two dummy vars that need to be sent to the gyroscope to call the getEvent function, they do nothing
  sensors_event_t accel;
  sensors_event_t temp;

  //Initialize Reading Loop
  if (!isRecording) {
    isRecording = true;
    startTime = millis();  // Record the starting time
    sample_index = 0;      // Reset index for new recording
  }

  //Data Collection and Period Calculation
  if (isRecording) {
    if (millis() - startTime < duration * 1000) {  //Collect data from
      sox.getEvent(&accel, &gyro, &temp);          //Sample data

      float data = 0;
      if (direction == 1) {  //Choose axis to get data from
        data = gyro.gyro.y;
      } else if (direction == 2) {
        data = gyro.gyro.x;
      } else if (direction == 3) {
        data = gyro.gyro.y;
      }

      //Save data and time for period calculation
      sensor_data[sample_index] = data;
      time_data[sample_index] = millis();
      sample_index++;

      //Fix sample rate
      delay(1000 / sampleRate);
    } else {
      isRecording = false;  //finish oscillation

      //Send data through averaging
      float filtered[arraySize];
      for (int i = 0; i < arraySize; i++) {
        filtered[i] = sensor_data[i];
      }
      moving_avg_filter(sensor_data, 5, filtered);

      //Get period, set oscillated variable = true, and and print to screen
      if (direction == 1) {
        x_period = get_period(filtered) / 1000;
        x_oscillated = true;
        inertia_x_oscillate();

      } else if (direction == 2) {
        y_period = get_period(filtered) / 1000;
        y_oscillated = true;
        inertia_y_oscillate();

      } else if (direction == 3) {
        z_period = get_period(filtered) / 1000;
        z_oscillated = true;
        inertia_z_oscillate();
      }
    }
  }
}

//Calculate the period by finding peaks in values
float get_period(float values[]) {

  int peakSize = 5;
  bool foundFirstPeak = false;
  unsigned long lastTime = 0;

  int numPeriods = 0;
  float periodSum = 0;

  for (int i = peakSize; i < arraySize - peakSize; i++) {

    if (detect_peak(values, i, peakSize)) {
      if (foundFirstPeak == false) {
        lastTime = time_data[i];  //Find the time in between the periods
        foundFirstPeak = true;    //first peak means don't subtract last time yet
      } else {
        unsigned long currTime = time_data[i];


        periodSum += currTime - lastTime;
        numPeriods++;  //calculate number of periods to do an average
        lastTime = currTime;
      }
    }
  }

  //Return average period
  return periodSum / numPeriods;
}

//Detect a peak by checking that centerIndex is greater than values +-size to its left and right
bool detect_peak(float values[], int centerIndex, int size) {
  float middleValue = values[centerIndex];

  for (int i = centerIndex - size; i <= centerIndex + size; i++) {
    if (i < 0 || i >= arraySize || i == centerIndex) {
      continue;
    }
    if (values[i] >= middleValue) {
      return false;  // Not a peak if any surrounding value is greater or equal
    }
  }
  return true;  // It's a peak if no surrounding value is greater
}

//Moving average filter that averages values in width size from values[] and fills in the filtered[] array
void moving_avg_filter(float values[], int size, float filtered[]) {
  int windowSize = 2 * size + 1;

  //Calculate the first average
  float sum = 0;
  for (int i = 0; i < windowSize; i++) {
    sum += values[i];
  }

  filtered[size] = sum / windowSize;

  //Complete the rest of the moving window operation
  for (int i = size + 1; i < arraySize - size; i++) {
    sum = sum - values[i - size - 1] + values[i + size];
    filtered[i] = sum / windowSize;
  }
}

//axis:
//1 -> x
//2 -> y
//3 -> z
float CalculateMMI(float period, float lo, int axis) {

  float platform_cog_distance = 0;
  float platform_period = 0;

  //Get the proper distance from platform cog to rotation point
  if (axis == 1 || axis == 3) {
    platform_cog_distance = platform_cog_distance_xz;
    platform_period = platform_period_xz;
  } else if (axis == 2) {
    platform_cog_distance = platform_cog_distance_y;
    platform_period = platform_period_y;
  }

  float pi_cons = 4 * pow(PI, 2);  //constant in equation

  float equation = model_weight_final * lo * ((pow(period, 2) / pi_cons) - (lo / g)) + ((platform_weight * platform_cog_distance) / pi_cons) * (pow(period, 2) - pow(platform_period, 2));

  return equation;
}

float GetFinalModelWeight(float frontWeight, float leftWeight, float rightWeight) {
  return (frontWeight + leftWeight + rightWeight) - (platform_front_weight + platform_left_weight + platform_right_weight);
}

//Calculate cg relative to referencePoint
float CalculateCG(float frontWeight, float leftWeight, float rightWeight, float referencePoint, int dir) {

  //total weight of the model - total weight of the platform : calibration
  float modelWeight = (frontWeight + leftWeight + rightWeight) - (platform_front_weight + platform_left_weight + platform_right_weight);
  float cg = 0;

  if (dir == 1) {  //x
    cg = ((frontWeight - platform_front_weight) * front_scale_pos[0] + (leftWeight - platform_left_weight) * left_scale_pos[0] + (rightWeight - platform_right_weight) * right_scale_pos[0]) / (modelWeight);
  } else if (dir == 2) {  //y
    cg = ((frontWeight - platform_front_weight) * front_scale_pos[1] + (leftWeight - platform_left_weight) * left_scale_pos[1] + (rightWeight - platform_right_weight) * right_scale_pos[1]) / (modelWeight);
  } else if (dir == 3) {  //z
    cg = ((frontWeight - platform_front_weight) * front_scale_pos[0] + (leftWeight - platform_left_weight) * left_scale_pos[0] + (rightWeight - platform_right_weight) * right_scale_pos[0]) / (modelWeight);
  }

  return cg - referencePoint;
}

//Save results to SD card once measurements are complete
void SaveToSDCard() {
  output_file = SD.open("output.txt", FILE_WRITE);

  if (output_file) {
    //PRINT COG
    output_file.print(x_cog_final);
    output_file.print(",");
    output_file.print(y_cog_final);
    output_file.print(",");
    output_file.print(z_cog_final);

    output_file.println();

    //PRINT MMI
    output_file.print(x_mmi_final);
    output_file.print(",");
    output_file.print(y_mmi_final);
    output_file.print(",");
    output_file.print(z_mmi_final);
  }

  output_file.close();
}
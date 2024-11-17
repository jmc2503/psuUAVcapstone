#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <string.h> // For strcpy and strncpy

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

void setup() {
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
  } else {
    lcd.clear();
    lcd.print("Selected XY Option: ");
    lcd.print(key);
    delay(1000);
    inertia_xy_screen(); // Redisplay Inertia XY screen
  }
}

void handleInertiaYZScreen(char key) {
  if (key == '#') {
    displayInertiaMenu(); // Go back to inertia menu
    menuState = 2;
  } else {
    lcd.clear();
    lcd.print("Selected YZ Option: ");
    lcd.print(key);
    delay(1000);
    inertia_yz_screen(); // Redisplay Inertia YZ screen
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

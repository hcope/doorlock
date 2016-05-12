#include <ESP8266.h>
#include <Keypad.h>
#include <Key.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define wifiSerial Serial1
#define serialYes true

ESP8266 wifi = ESP8266(true);

//following lines from AdaFruit's example.h  
//this sets up the keypad
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {23, 22, 21, 20}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {16, 15, 14}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS );
//this ends the sample code

String passcode;
int SolenoidPin = 9;
int LedPin = 13;
int failed_attempts = 0;
int allowed_fails = 1;

//The 6.s08 definitions from L02A
#define DISPLAYUPDATEINTERVAL 100
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif
//end of 6.s08 code

void passcode_fail(){
  /*
  This is a wrapper function to handle when we fail a passcode.
  We hope to prevent repetition of code.
  */
  failed_attempts ++;
  if(failed_attempts > allowed_fails){
    failed_attempts = 0;
    camera_setup();
    }
  }

String enter_code(){
  /* 
  This is a helper function whi ch loops in order to add digits to a string
  Exits upon pressing the asterisk key.
  */
  char key = keypad.getKey();
  String char_string = "";
  while(key != '*'){
    if(key != 0){
      if(key == '#'){
        return "";}
      else{
        char_string.append(String(key));
        display.print(String(key));
        display.display();
      }}
    delay(100);
    key = keypad.getKey();
    }
  return char_string;
  }

boolean attempting_state(String code = passcode){
  /*
  This function is triggered when we determine that the user is attemping to enter a password
  It then loops until it finds the star key, and then decides if the password being entered matches the saved passcode
  If it does, return true, else false.
  */
  String attempt = enter_code();
  if(attempt != code and attempt != ""){
    passcode_fail();
    //wrapper because anytime we attempt the password, either to change it or open the lock, we want the same failure mode, not true for passing
    //note that if we exit with an empty string we don't want that string to count as an attempt.
    }
  return (attempt == code);
  }

boolean update_state(){
  /*
  This is a function which handles the updating of the passcode.
  */
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Please enter your current passcode.");
  display.display();
  delay(500);
  boolean correct_passcode = attempting_state(); //calls the default with the passcode
  if (correct_passcode){
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Enter your updated passcode.");
    display.display();
    String new_passcode = enter_code();
    if(new_passcode == "" or new_passcode.length() > 16){
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Passcodes must contain between 1 and 16 characters.");
      display.display();
      delay(2000);
      //Serial.println("Passcodes must contain between 1 and 16 characters.");
      return false;}
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Re-enter your new password");
    display.display();
    delay(1000);
    boolean can_update = attempting_state(new_passcode);
    if(can_update){
      setup_wifi();
      send_combo(new_passcode);
      failed_attempts = 0;
      return true;
      }
    else{
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Codes do not match. Update failed.");
      display.display();
      delay(2000);
      return false;}
    }
  else{
    return false;
    }  
  }

void setup() { //initializes display
  pinMode(SolenoidPin, OUTPUT);
  pinMode(LedPin, OUTPUT);
  setup_wifi();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Starting doorlock");
  display.display();
  passcode = load_combo();
  delay(2000);
}

void save_energy(bool wifiOff){
  /*
   This function turns off various lights and components when called in order to save battery.
   */
  digitalWrite(LedPin, LOW);
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  delay(500);
  if(wifiOff){
    digitalWrite(2,LOW);
  }
}

void loop() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Begin entering passcode by using *. Use # to update passcode.");
  display.display();
  delay(500);
  char key = keypad.getKey();
  if(key == '#')
    {
    boolean updated = update_state(); 
    if(updated){
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Passcode updated and is now ");
      display.print(passcode);
      display.display();
      delay(2000);
      digitalWrite(2, LOW);
      }
    else{
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Passcode update failed.");
      display.display();
      delay(2000);
      }
    }
  else if (key == '*')
    {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Attempting passcode (end with *):");
    display.display();
    delay(1000);
    boolean unlocked = attempting_state();
    if (unlocked)
      {
      failed_attempts = 0;
      display.println("\n Congrats, you're in the mainframe.");
      display.display();
      for (int i = 0; i < 30; i++)
      {
         // Flash the led to indicate the lock is open
         digitalWrite(SolenoidPin, HIGH);
         digitalWrite(LedPin, LOW);
         delay(50);
         digitalWrite(LedPin, HIGH);
         delay(50);
      }
      digitalWrite(SolenoidPin, LOW);
      }
    }
    delay(100);
  }

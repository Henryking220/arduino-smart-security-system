#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// =============================
// LCD
// =============================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =============================
// PINS
// =============================
#define TRIG_PIN 7
#define ECHO_PIN 6
#define PIR_PIN 8
#define LED_PIN 5
#define BUZZER_PIN 4

// =============================
// KEYPAD
// =============================
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {2, 3, 9, 10};
byte colPins[COLS] = {11, 12, 13, A0};

Keypad keypad = Keypad(
  makeKeymap(keys),
  rowPins,
  colPins,
  ROWS,
  COLS
);

// =============================
// SECURITY
// =============================
const String correctPIN = "2519";

String enteredPIN = "";

bool systemArmed = false;
bool enteringPIN = false;
bool armingMode = false;

// =============================
// TIMERS
// =============================
unsigned long lastMotionTime = 0;
unsigned long lastBeepTime = 0;
unsigned long lastDistanceTime = 0;
unsigned long lastLCDTime = 0;

const unsigned long LIGHT_TIMEOUT = 15000;
const unsigned long DISTANCE_INTERVAL = 100;
const unsigned long LCD_INTERVAL = 250;

// =============================
// SENSOR DATA
// =============================
float distance = 999;
bool motionDetected = false;

// =============================
// SETUP
// =============================
void setup() {

  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(PIR_PIN, INPUT);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("SMART ROOM");
  lcd.setCursor(0, 1);
  lcd.print("SECURITY SYSTEM");

  delay(2000);

  showMainScreen();
}

// =============================
// MAIN LOOP
// =============================
void loop() {

  handleKeypad();

  readPIR();

  updateDistance();

  updateLight();

  updateBuzzer();

  updateLCD();

}

// =============================
// KEYPAD
// =============================
void handleKeypad() {

  char key = keypad.getKey();

  if (!key) {
    return;
  }

  Serial.print("Key: ");
  Serial.println(key);

  // -------------------------
  // Start ARM
  // -------------------------
  if (key == 'A') {

    enteringPIN = true;
    armingMode = true;
    enteredPIN = "";

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ARM SYSTEM");
    lcd.setCursor(0, 1);
    lcd.print("PIN:");

    return;
  }

  // -------------------------
  // Start DISARM
  // -------------------------
  if (key == 'B') {

    enteringPIN = true;
    armingMode = false;
    enteredPIN = "";

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DISARM SYSTEM");
    lcd.setCursor(0, 1);
    lcd.print("PIN:");

    return;
  }

  // -------------------------
  // PIN MODE
  // -------------------------
  if (enteringPIN) {

    // Clear
    if (key == '*') {

      enteredPIN = "";

      lcd.setCursor(4, 1);
      lcd.print("            ");

      lcd.setCursor(4, 1);

      return;
    }

    // Number
    if (key >= '0' && key <= '9') {

      if (enteredPIN.length() < 6) {

        enteredPIN += key;

        lcd.setCursor(
          4 + enteredPIN.length() - 1,
          1
        );

        lcd.print("*");
      }

      return;
    }

    // Submit
    if (key == '#') {

      checkPIN();

      return;
    }
  }

  // -------------------------
  // STATUS
  // -------------------------
  if (key == 'D') {

    lcd.clear();

    if (systemArmed) {

      lcd.setCursor(0, 0);
      lcd.print("SYSTEM: ARMED");

    } else {

      lcd.setCursor(0, 0);
      lcd.print("SYSTEM: OFF");
    }

    lcd.setCursor(0, 1);
    lcd.print("D:");
    lcd.print(distance, 1);
    lcd.print("cm");

    lastLCDTime = millis();

    return;
  }
}

// =============================
// CHECK PIN
// =============================
void checkPIN() {

  if (enteredPIN == correctPIN) {

    if (armingMode) {

      systemArmed = true;

      Serial.println("SYSTEM ARMED");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ACCESS GRANTED");
      lcd.setCursor(0, 1);
      lcd.print("SYSTEM ARMED");

    } else {

      systemArmed = false;

      digitalWrite(LED_PIN, LOW);
      noTone(BUZZER_PIN);

      Serial.println("SYSTEM DISARMED");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ACCESS GRANTED");
      lcd.setCursor(0, 1);
      lcd.print("SYSTEM OFF");
    }

  } else {

    Serial.println("WRONG PIN");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ACCESS DENIED");
    lcd.setCursor(0, 1);
    lcd.print("WRONG PIN");

    tone(BUZZER_PIN, 800);

    delay(250);

    noTone(BUZZER_PIN);
  }

  enteredPIN = "";
  enteringPIN = false;

  lastLCDTime = millis();
}

// =============================
// PIR
// =============================
void readPIR() {

  motionDetected = digitalRead(PIR_PIN);

  if (motionDetected) {

    lastMotionTime = millis();

    if (systemArmed) {
      digitalWrite(LED_PIN, HIGH);
    }
  }
}

// =============================
// DISTANCE
// =============================
void updateDistance() {

  if (millis() - lastDistanceTime < DISTANCE_INTERVAL) {
    return;
  }

  lastDistanceTime = millis();

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(
    ECHO_PIN,
    HIGH,
    30000
  );

  if (duration == 0) {

    distance = 999;

  } else {

    distance = duration * 0.0343 / 2;
  }
}

// =============================
// LIGHT
// =============================
void updateLight() {

  if (!systemArmed) {

    digitalWrite(LED_PIN, LOW);
    return;
  }

  if (
    digitalRead(LED_PIN) == HIGH &&
    millis() - lastMotionTime >= LIGHT_TIMEOUT
  ) {

    digitalWrite(LED_PIN, LOW);
  }
}

// =============================
// BUZZER
// =============================
void updateBuzzer() {

  if (!systemArmed || enteringPIN) {

    noTone(BUZZER_PIN);
    return;
  }

  // Very close
  if (distance < 30) {

    tone(BUZZER_PIN, 2000);
  }

  // Close
  else if (distance < 75) {

    if (millis() - lastBeepTime >= 300) {

      lastBeepTime = millis();

      tone(BUZZER_PIN, 1500);
    }

    if (millis() - lastBeepTime >= 100) {

      noTone(BUZZER_PIN);
    }
  }

  // Medium
  else if (distance < 150) {

    if (millis() - lastBeepTime >= 1000) {

      lastBeepTime = millis();

      tone(BUZZER_PIN, 1000);
    }

    if (millis() - lastBeepTime >= 100) {

      noTone(BUZZER_PIN);
    }
  }

  // Far
  else {

    noTone(BUZZER_PIN);
  }
}

// =============================
// LCD
// =============================
void updateLCD() {

  if (enteringPIN) {
    return;
  }

  if (millis() - lastLCDTime < LCD_INTERVAL) {
    return;
  }

  lastLCDTime = millis();

  lcd.setCursor(0, 0);

  if (systemArmed) {

    if (motionDetected) {
      lcd.print("MOTION DETECTED");
    } else {
      lcd.print("SYSTEM ARMED   ");
    }

  } else {

    lcd.print("SYSTEM DISARMED");
  }

  lcd.setCursor(0, 1);

  lcd.print("D:");

  if (distance >= 999) {
    lcd.print("---");
  } else {
    lcd.print(distance, 1);
  }

  lcd.print("cm ");

  if (digitalRead(LED_PIN)) {
    lcd.print("L:ON ");
  } else {
    lcd.print("L:OFF");
  }
}

// =============================
// MAIN SCREEN
// =============================
void showMainScreen() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("SYSTEM DISARMED");

  lcd.setCursor(0, 1);
  lcd.print("A=ARM B=OFF");
}
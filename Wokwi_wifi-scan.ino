// Wokwi simulation settings:
// https://wokwi.com/projects/new
// Required libraries:
#include <ESP32Servo.h>
#include <HX711.h>
#include <LiquidCrystal_I2C.h>

// Servo motor
Servo servo;
const int SERVO_PIN = 13;

// Stepper motor via A4988
const int STEP_PIN = 14;
const int DIR_PIN = 12;
const int STEPS_180 = 100;  // Half rotation steps (tune if needed)

// HX711 load cell
HX711 scale;
const int HX_DT = 32;
const int HX_SCK = 33;

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ✅ Only 3 valid RFID tags allowed
const String validTags[] = {"pet123", "dog007", "cat001"};
const int numTags = 3;

bool isAuthorizedTag(String tag) {
  for (int i = 0; i < numTags; i++) {
    if (tag == validTags[i]) return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial.println("System initialized. Waiting for authorized RFID...");

  // Servo setup
  servo.attach(SERVO_PIN);
  servo.write(0);  // initial position

  // Stepper setup
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  // HX711 setup
  scale.begin(HX_DT, HX_SCK);

  // LCD setup
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for pet");
}

void rotateStepper(int steps, bool dir) {
  digitalWrite(DIR_PIN, dir);
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(800);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(800);
  }
}

void loop() {
  if (Serial.available()) {
    String rfid = Serial.readStringUntil('\n');
    rfid.trim();

    if (rfid.length() > 0) {
      if (isAuthorizedTag(rfid)) {
        Serial.println("✅ Authorized pet: " + rfid);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Pet Detected!");

        // Move Servo to 90°
        servo.write(90);
        delay(5000); // wait 5 seconds
        servo.write(0); // return to original position

        // Rotate stepper motor 180°
        rotateStepper(STEPS_180, true);

        lcd.setCursor(0, 1);
        lcd.print("Fed & Logged");
        delay(3000);
      } else {
        Serial.println("❌ Unauthorized tag: " + rfid);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Denied");
        lcd.setCursor(0, 1);
        lcd.print("Unknown Tag");
        delay(3000);
      }

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Waiting for pet");
    }
  }

  delay(100);
}

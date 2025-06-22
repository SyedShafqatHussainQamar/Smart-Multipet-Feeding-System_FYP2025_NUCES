//----------------------------------Wifi----------------------------------------
#include <WiFi.h>
#include <HTTPClient.h>

// Replace with your WiFi credentials
const char* ssid = "Infinix";
const char* password = "123456789a";

// ThingSpeak settings
const char* server = "http://api.thingspeak.com/update";
const String apiKey = "OUR9PR007SGPVQVO";  // Replace with your ThingSpeak write key
//----------------------------------------------------------------------------
//-------------------------------------------flags and cunter------------------------------
bool  motion_detected=false;
float sensorValue = 0; // Simulated sensor value
long reading=0;
// ----------------------------------------------------------------------------------------------------
#include "HX711.h"
#include <ESP32Servo.h>
bool first_rotation=false;
bool sr=false;
Servo myServo;  // Create a servo object to control the servo
// Define the pin connected to the servo control wire
int servoPin = 13;  // Change the pin number if necessary

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 32;
const int LOADCELL_SCK_PIN = 33;
HX711 scale;

TaskHandle_t Task2motor = NULL;
TaskHandle_t Task2;
int x=0;
#include <AccelStepper.h>
#include <SPI.h>
#include <MFRC522.h>
#define motorInterfaceType 1
#define SS_PIN 5
#define RST_PIN 0
const int DIR = 12;
const int STEP = 14;



// Timer variables
unsigned long lastTime = 0;
unsigned long servoTime = 0;

unsigned long reset_rfid = 0;
unsigned long rfid_timer_compare = 0;
unsigned long timerDelay = 15000;
unsigned long reset_rfid_delay = 50000;

int weight=0;
AccelStepper motor(motorInterfaceType, STEP, DIR);
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];

void motor_task(void *pvParameters) {
  while(1){
    motor.run();
    vTaskDelay(20);
}
}


void sendDataTask(void* parameter) {

 WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
  if(reading>10000){
    reading=1000;
  }
      String url = String(server) + "?api_key=" + apiKey +
             "&field1=" + String(motion_detected ? 1 : 0) +         // Convert bool to int
             "&field2=" + String(sensorValue)+
             "&field3=" + String(reading, 2); 

      http.begin(url);
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        Serial.print("✅ Data sent! HTTP Response code: ");
        Serial.println(httpResponseCode);
      } else {
        Serial.print("❌ Error sending data: ");
        Serial.println(httpResponseCode);
      }
    if(motion_detected ==true){
      motion_detected=false;
    }
      http.end();
    } else {
      Serial.println("🔁 WiFi not connected. Reconnecting...");
      WiFi.begin(ssid, password);
    }

    vTaskDelay(15000 / portTICK_PERIOD_MS);  // Delay 15 seconds
    if(sensorValue>3){
      sensorValue=0;
    }

  }
}

void setup() { 
  Serial.begin(9600);
myServo.attach(servoPin); // Min and max pulse widths in µs


scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

 
            
  scale.set_scale(75.8095);
  //scale.set_scale(-471.497);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();               // reset the scale to 0

  Serial.println("After setting up the scale:");




  motor.setMaxSpeed(500);
  motor.setAcceleration(250);
  motor.setSpeed(100);
  motor.moveTo(0);

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);














//  xTaskCreatePinnedToCore(
//     Task1Stepper,         // Task function
//     "Stepper Task",       // Task name
//     4096,                 // Stack size
//     NULL,                 // Parameters
//     1,                    // Priority
//     &Task1StepperHandle,  // Task handle
//     1                     // Core 1
//   );

  xTaskCreatePinnedToCore(
    motor_task,           // Task function
    "Blink Task",         // Task name
    2048,                 // Stack size
    NULL,                 // Parameters
    1,                    // Priority
    &Task2motor,    // Task handle
    0                     // Core 0
  );

  xTaskCreatePinnedToCore(
    sendDataTask,   // Function
    "SendData",     // Name
    4096,           // Stack size
    NULL,           // Parameters
    1,              // Priority
    &Task2,         // Handle
    1               // Core
  );




}
 
void loop() {
  
 reading = scale.get_units();  // Default is 1 sample
  Serial.print("Weight: ");
  Serial.print(reading, 1); // one decimal place
  Serial.println(" units"); // You'll need to calibrate to get real grams or kg

  
myServo.write(0);
 
  
  if ((millis() - rfid_timer_compare) > 70000){
  nuidPICC[1]=151515;
  }

// lastTime = millis();
  // scale.power_up();

  
 MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent()){
  //  motion_detected=0;
  
// return;
  }
  // Verify if the NUID has been readed
 else if ( ! rfid.PICC_ReadCardSerial()){
    // motion_detected=0;

    // return;
  }

  // Check is the PICC of Classic MIFARE type
  else if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && 
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    // motion_detected=0;
    // return;
  }

  else if (rfid.uid.uidByte[0] != nuidPICC[0] || rfid.uid.uidByte[1] != nuidPICC[1] || rfid.uid.uidByte[2] != nuidPICC[2] || rfid.uid.uidByte[3] != nuidPICC[3]) {
  rfid_timer_compare = millis();
  Serial.println(F("A new card has been detected."));
   sensorValue ++; // Simulated sensor value
   motion_detected=true;
    // myServo.write(0);  // Move the servo to the current position

  // first_rotation=true;


  if (first_rotation == true) {
    if (motor.distanceToGo() == 0) {
    motor.moveTo(motor.currentPosition() + 66); 
    delay(3000);
    myServo.write(80);
    delay(5000);
    }
  }
  else{
    myServo.write(80);
    delay(5000);
  }
  first_rotation=true;
 

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }

  Serial.println(F("The NUID tag is:"));
  Serial.print(F("In hex: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();
  Serial.print(F("In dec: "));
  printDec(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();
  // lastTime = millis();

}
else {
  //  motion_detected=0;

  Serial.println(F("Card read previously."));
  motion_detected = 0;

}
// Halt PICC
rfid.PICC_HaltA();

// Stop encryption on PCD
rfid.PCD_StopCrypto1();

}


/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}






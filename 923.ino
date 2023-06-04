#include <Arduino.h>
#include <Hash.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SHA256.h>
#include <ArduinoJson.h> 

// Replace with your network credentials
const char* ssid = "Skaiks iPhone";
const char* password = "112233445566";

// Replace with your server URL
const String serverUrl = "http://172.20.10.2:5000/arduino_only123321";

// Initialize the software serial port for fingerprint sensor communication
SoftwareSerial fingerSerial(D6, D5); // RX, TX

// Create an instance of the Adafruit Fingerprint sensor
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

// Create an instance of the LiquidCrystal_I2C library for LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int imageBufferSize = 256; // Define the size of the image buffer

// Variables to store the registered and verified fingerprint values
String registeredFingerprintValue = "";
String verifiedFingerprintValue = "";

void setup() {
  // Start serial communication
  Serial.begin(115200);
  
  // Initialize the fingerprint sensor
  finger.begin(57600);
  
  // Initialize the LCD display
  lcd.begin();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to WiFi");
  
  showMainMenu();
}

void loop() {
  // Main menu loop
  while (true) {
    if (Serial.available()) {
      char option = Serial.read();
      
      switch (option) {
        case '1':
          registerFingerprint();
          break;
        case '2':
          verifyFingerprint();
          break;
        default:
          break;
      }
    }
  }
}

void showMainMenu() {
  lcd.clear();
  lcd.print("1. Register");
  lcd.setCursor(0, 1);
  lcd.print("2. Verify");
  Serial.println("Main Menu");
}

void sendHttpPostRequest(String url, String contentType, String payload) {
  WiFiClient client;
  HTTPClient http;

  // Specify the target URL and headers
  http.begin(client, url);
  http.addHeader("Content-Type", contentType);

  // Send the POST request and handle the response
  int httpResponseCode = http.POST(payload);
  String responsePayload = http.getString();

  // Print the response code and payload for debugging
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  Serial.print("Response payload: ");
  Serial.println(responsePayload);

  // End the HTTP connection
  http.end();
}

void registerFingerprint() {
  lcd.clear();
  lcd.print("Registering...");

  // Code for registering a fingerprint
  Serial.println("Place your finger on the sensor...");
  int fingerprintID = -1;
  while (fingerprintID != FINGERPRINT_OK) {
    fingerprintID = finger.getImage();

    switch (fingerprintID) {
      case FINGERPRINT_OK:
        {
          Serial.println("Image taken");
          uint8_t imageBuffer[imageBufferSize];
          finger.image2Tz(1);
          finger.fingerFastSearch();
          Serial.print("Fingerprint registration value: ");
          for (int i = 0; i < imageBufferSize; i++) {
            String hexValue = String(imageBuffer[i], HEX);
            Serial.print(hexValue);
            Serial.print(' ');
          }
          Serial.println();

          // Convert the fingerprint value to a string
          String fingerprintValue;
          for (int i = 0; i < imageBufferSize; i++) {
            String hexValue = String(imageBuffer[i], HEX);
            fingerprintValue += hexValue;
            fingerprintValue += " ";
          }
          
          // Hash the fingerprint value
          String hashedValue = calculateHash(fingerprintValue);

          // Print the hashed fingerprint value
          Serial.print("Hashed Fingerprint value: ");
          Serial.println(hashedValue);

          // Store the registered fingerprint value
          registeredFingerprintValue = hashedValue;

          lcd.clear();
          lcd.print("Fingerprint");
          lcd.setCursor(0, 1);
          lcd.print("registered");
          delay(2000);
          showMainMenu();
          break;
        }
      case FINGERPRINT_NOFINGER:
        Serial.println("No finger detected");
        delay(500);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        delay(500);
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        delay(500);
        break;
      default:
        Serial.println("Unknown error");
        delay(500);
        break;
    }
  }
}

void verifyFingerprint() {
  lcd.clear();
  lcd.print("Verifying...");

  // Code for verifying a fingerprint
  Serial.println("Place your finger on the sensor...");
  int fingerprintID = -1;
  while (fingerprintID != FINGERPRINT_OK) {
    fingerprintID = finger.getImage();

    switch (fingerprintID) {
      case FINGERPRINT_OK:
        {
          Serial.println("Image taken");
          uint8_t imageBuffer[imageBufferSize];
          finger.image2Tz(1);
          finger.fingerFastSearch();
          Serial.print("Fingerprint value: ");
          for (int i = 0; i < imageBufferSize; i++) {
            String hexValue = String(imageBuffer[i], HEX);
            Serial.print(hexValue);
            Serial.print(' ');
          }
          Serial.println();

          lcd.clear();
          lcd.print("Verifying...");

          // Convert the fingerprint value to a string
          String fingerprintValue;
          for (int i = 0; i < imageBufferSize; i++) {
            String hexValue = String(imageBuffer[i], HEX);
            fingerprintValue += hexValue;
            fingerprintValue += " ";
          }
          
          // Hash the fingerprint value
          String hashedValue = calculateHash(fingerprintValue);

          // Print the hashed fingerprint value and the fingerprint value
          Serial.print("Hashed Fingerprint value: ");
          Serial.println(hashedValue);
          Serial.print("Fingerprint value verification: ");
          Serial.println(fingerprintValue);

          // Store the verified fingerprint value
          verifiedFingerprintValue = hashedValue;

          // Create a JSON object for the request payload
          StaticJsonDocument<256> doc;
          doc["fingerprint"] = hashedValue;
          doc["busID"] = "4";

          // Serialize the JSON object to a string
          String payload;
          serializeJson(doc, payload);

          // Send the HTTP POST request
          sendHttpPostRequest(serverUrl, "application/json", payload);

          lcd.clear();
          if (registeredFingerprintValue == verifiedFingerprintValue) {
            lcd.print("Fingerprint");
            lcd.setCursor(0, 1);
            lcd.print("matched");
          } else {
            lcd.print("Fingerprint");
            lcd.setCursor(0, 1);
            lcd.print("not matched");
          }

          delay(2000);
          showMainMenu();
          break;
        }
      case FINGERPRINT_NOFINGER:
        Serial.println("No finger detected");
        delay(500);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        delay(500);
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        delay(500);
        break;
      default:
        Serial.println("Unknown error");
        delay(500);
        break;
    }
  }
}

#define SHA256_HASH_SIZE 32

String calculateHash(String input) {
  SHA256 sha256;

  // Convert the input string to a char array
  char charArray[input.length() + 1];
  input.toCharArray(charArray, sizeof(charArray));

  // Calculate the hash
  uint8_t hash[SHA256_HASH_SIZE];
  sha256.reset();
  sha256.update(charArray, input.length());
  sha256.finalize(hash, SHA256_HASH_SIZE);

  // Convert the hash to a string
  String hashedValue;
  for (int i = 0; i < SHA256_HASH_SIZE; i++) {
    String hexValue = String(hash[i], HEX);
    if (hexValue.length() == 1) {
      hashedValue += '0';
    }
    hashedValue += hexValue;
  }

  return hashedValue;
}

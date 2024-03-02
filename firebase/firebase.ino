#include <Wire.h>
#include <SPI.h>
#include <RFID.h>
#include "Firebase_ESP_Client.h" // Use Firebase ESP Client library
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>


// Constants
#define DEBOUNCE_DELAY 2000 // 2000 milliseconds debounce period

// Global variables
unsigned long lastCardReadTime = 0; // Timestamp of the last RFID card read

// Firebase project credentials
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

#define FIREBASE_HOST "clockinout-6bb64-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyBE-dkBFP83O6PSgXvzOTyloh8FnQr9Tpc"

RFID rfid(5, 22); // Adjust pin numbers according to your setup
unsigned char str[MAX_LEN]; // MAX_LEN is 16

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 7200; // UTC+5:30
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

const char ssid[] = "Georgi";
const char pass[] = "gogo1701";

String uidPath = "/";
FirebaseJson json;

const int red = 4;
const int green = 2;
String device_id = "device1";
boolean checkIn = true;

void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void setup() {
  Serial.begin(115200);

  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);

  SPI.begin();
  rfid.init();

  connectWiFi();

  timeClient.begin();

  config.host = "clockinout-6bb64-default-rtdb.europe-west1.firebasedatabase.app";
  config.api_key = "AIzaSyBE-dkBFP83O6PSgXvzOTyloh8FnQr9Tpc";
  // Initialize Firebase
    if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign-up OK");
  } else {
    Serial.printf("Sign-up error: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void checkAccess(String temp) {
  if (Firebase.RTDB.getInt(&firebaseData, uidPath + "/users/" + temp)) {
    json.clear();
    json.add("time", timeClient.getFormattedTime());
    json.add("id", device_id);
    json.add("uid", temp);
    json.add("status", firebaseData.intData() == 0 ? 1 : 0);

    // Update the user's check-in status
    Firebase.RTDB.setInt(&firebaseData, uidPath + "/users/" + temp, firebaseData.intData() == 0 ? 1 : 0);
    
    // Attempt to push the JSON data to Firebase
    if (!Firebase.RTDB.pushJSON(&firebaseData, uidPath + "/attendance", &json)) {
      Serial.println(firebaseData.errorReason());
    }
  } else {
    Serial.println("FAILED");
    Serial.println("REASON: " + firebaseData.errorReason());
  }
}


void loop() {
  timeClient.update();
  unsigned long currentMillis = millis();

  // Check if we are outside of the debounce delay
  if (currentMillis - lastCardReadTime > DEBOUNCE_DELAY) {
    if (rfid.findCard(PICC_REQIDL, str) == MI_OK) {
      Serial.println("Card found");
      String temp = "";

      if (rfid.anticoll(str) == MI_OK) {
        for (int i = 0; i < 4; i++) {
          temp += String(str[i] >> 4, HEX);
          temp += String(str[i] & 0x0F, HEX);
        }
        Serial.println(temp);
        checkAccess(temp);
        
        // Update the last card read time
        lastCardReadTime = currentMillis;
      }
      rfid.selectTag(str);
    }
  }
}

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

// Firebase and WiFi configuration
#define WIFI_SSID "Dai Shin Kai"
#define WIFI_PASSWORD "123456879"
#define API_KEY "AIzaSyAfRq1L3pQckPz9yx8UFVLtM9CtUwoIWQM"
#define DATABASE_URL "smarthome-5a747-default-rtdb.firebaseio.com"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Pin configuration
#define SS_PIN  D8 // The ESP8266 pin D8
#define RST_PIN D2 // The ESP8266 pin D2
#define SERVO_PIN D0 // The ESP8266 pin connects to servo motor
#define GREEN_LED_PIN D1 // The ESP8266 pin for the green LED
#define RED_LED_PIN D3 // The ESP8266 pin for the red LED

MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo;

std::vector<String> authorizedTags; // Vector to store authorized tag IDs
int angle = 0; // The current angle of servo motor
unsigned long previousMillis = 0;
const long interval = 3000; // 3 seconds interval
unsigned long redLedOffTime = 0;
bool redLedOn = false;


void setup() {
  Serial.begin(9600);
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522
  servo.attach(SERVO_PIN);
  servo.write(angle); // rotate servo motor to 0°
  pinMode(GREEN_LED_PIN, OUTPUT); // set the green LED pin as output
  pinMode(RED_LED_PIN, OUTPUT); // set the red LED pin as output

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("Connected with IP: " + WiFi.localIP().toString());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign-up failed, reason: " + fbdo.errorReason());
  }

  // Fetch and store authorized tags from Firebase
  if (Firebase.RTDB.getJSON(&fbdo, "/AuthorizedTags")) {
    if (fbdo.dataType() == "json") {
      FirebaseJson* json = fbdo.jsonObjectPtr();
      size_t len = json->iteratorBegin();
      String key, value;
      int type = 0;
      for (size_t i = 0; i < len; i++) {
        json->iteratorGet(i, type, key, value);
        value.toUpperCase(); // Convert to upper case for consistent comparison
        authorizedTags.push_back(value);
        Serial.println("Authorized tag loaded: " + value);
      }
      json->iteratorEnd();
    }
  }

  Serial.println("Tap RFID/NFC Tag on reader");
}

void loop() {
    unsigned long currentMillis = millis();
     if (redLedOn && currentMillis >= redLedOffTime) {
        digitalWrite(RED_LED_PIN, LOW); // Turn off red LED
        redLedOn = false; // Reset the flag
    }

    if (currentMillis - previousMillis >= interval) {
        servo.write(0);
        digitalWrite(GREEN_LED_PIN, LOW);
    }

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String readTag = "\"";  // Start with a double quote
        for (byte i = 0; i < rfid.uid.size; i++) {
            if (rfid.uid.uidByte[i] < 0x10) {
                readTag += "0";
            }
            readTag += String(rfid.uid.uidByte[i], HEX);
        }
        readTag.toUpperCase();
        readTag += "\"";  // End with a double quote
        Serial.println("Tag read: " + readTag);

        // Direct comparison debug
        for (auto& tag : authorizedTags) {
            Serial.print("Comparing with: ");
            Serial.println(tag);
            if (tag == readTag) {
                Serial.println("Direct comparison succeeded.");
            } else {
                Serial.println("Direct comparison failed.");
            }
        }

        // Adjust this line if authorizedTags are stored without quotes
        // For a valid comparison, ensure the tags in authorizedTags are stored in the same format
        if (std::find(authorizedTags.begin(), authorizedTags.end(), readTag) != authorizedTags.end()) {
            Serial.println("Authorized Tag: " + readTag);
            digitalWrite(GREEN_LED_PIN, HIGH);
            digitalWrite(RED_LED_PIN, LOW);
            angle = 90;
            servo.write(angle);
            Serial.print("Rotate Servo Motor to ");
            Serial.print(angle);
            Serial.println("°");
            previousMillis = currentMillis;
        } else {
            Serial.println("Unauthorized Tag: " + readTag);
            digitalWrite(RED_LED_PIN, HIGH);
            digitalWrite(GREEN_LED_PIN, LOW);
              redLedOn = true;
            redLedOffTime = currentMillis + 2000;
        }
        rfid.PICC_HaltA(); // halt PICC
        rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
}

#include "DHT.h"
#define DHTPIN 4
#define DHTTYPE DHT11

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

DHT dht(DHTPIN, DHTTYPE);

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "OPPO Reno6"
#define WIFI_PASSWORD "rorororo"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAfRq1L3pQckPz9yx8UFVLtM9CtUwoIWQM"

// Insert RTDB URL
#define DATABASE_URL "smarthome-5a747-default-rtdb.firebaseio.com" 
#define LED D2

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

void setup(){
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);  // Éteindre la LED au démarrage
  
  pinMode(DHTPIN, INPUT);
  dht.begin();
  
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (Firebase.ready() && signupOK) {
    if (Firebase.RTDB.setFloat(&fbdo, "/Living Room/humidite", h)) {
      Serial.print("Humidity: ");
      Serial.println(h);
    } else {
      Serial.println("FAILED to write humidity data");
      Serial.println("REASON: " + fbdo.errorReason());
      Serial.println("Error: " + String(fbdo.errorCode()));
    }

    if (Firebase.RTDB.setFloat(&fbdo, "/Living Room/temperature", t)) {
      Serial.print("Temperature: ");
      Serial.println(t);
    } else {
      Serial.println("FAILED to write temperature data");
      Serial.println("REASON: " + fbdo.errorReason());
      Serial.println("Error: " + String(fbdo.errorCode()));
    }
  } else {
    Serial.println("Firebase not ready or signup failed");
  }
  Serial.println("______________________________");
  delay(5000);  // Ajoutez un délai pour ne pas surcharger Firebase
}


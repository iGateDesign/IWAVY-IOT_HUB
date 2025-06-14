/*************************************************************
  Blynk is a platform with iOS and Android apps to control
  ESP32, Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build mobile and web interfaces for any
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: https://www.blynk.io
    Sketch generator:           https://examples.blynk.cc
    Blynk community:            https://community.blynk.cc
    Follow us:                  https://www.fb.com/blynkapp
                                https://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on ESP8266 chip.

  NOTE: This requires ESP8266 support package:
    

  Please be sure to select the right ESP8266 module
  in the Tools -> Board menu!
  
 *************************************************************/
#include <Arduino.h>

#define BLYNK_TEMPLATE_ID "TMPL2GLIVNWgs"
#define BLYNK_TEMPLATE_NAME "Driveway Gate 1"
#define BLYNK_AUTH_TOKEN "JI43MeaJdWhFueimsOxv_jpD594UhKkB"

#define BLYNK_PRINT Serial

#if defined(ESP8266)
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
#endif

#include <WiFiClient.h>

#if defined(ESP8266)
    #include <BlynkSimpleEsp8266.h>
#elif defined(ESP32)
    #include <BlynkSimpleEsp32.h>
#endif

#include <Wire.h>
#include <AHTxx.h>
#include <WiFiManager.h>
#if defined(ESP8266)
    #include <ESP8266HTTPClient.h>
#elif defined(ESP32)
    #include <HTTPClient.h>
#endif

// Define the relay pin 
#if defined(ESP8266)
    #define RELAY_PIN 12  // GPIO12 (D6)
    #define LED_PIN   2   // GPIO2 (D4)
    #define SDA_PIN   4   // GPIO4 (D2)
    #define SCL_PIN   5   // GPIO5 (D1)
#elif defined(ESP32)
    #define RELAY_PIN 23  // Adjust for ESP32 equivalent pin
    #define LED_PIN   22
    #define SDA_PIN   21
    #define SCL_PIN   19
#endif

// Define the pulse duration in milliseconds (e.g., 1 second)
#define PULSE_DURATION 1000

// Create an instance of the AHTxx sensor
AHTxx aht(AHTXX_ADDRESS_X38, AHT2x_SENSOR); // AHT25 sensor
bool sensorPresent = false;

void setup() {
  // Start Serial communication
  Serial.begin(115200);
  Serial.println("Booting...");

  // Initialize the relay pin as an output
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // Initialize relay as off (since it is low-active)

  // Initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // Initialize the LED as off
  Serial.println("LED pin initialized");

  // Initialize the AHT25 sensor
  Wire.begin(SDA_PIN, SCL_PIN);
  sensorPresent = aht.begin();

  if (!sensorPresent) {
    Serial.println("AHT25 sensor not found, skipping sensor readings.");
  }

  // Wi-Fi Manager to handle credentials
  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("AutoConnectAP")) {
    Serial.println("Failed to connect");
    ESP.restart();
  }

  // Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());

  // Send Wi-Fi status to Bubble app
  sendWiFiStatus();
}

void loop() {
  Serial.println("Entering loop");

  Blynk.run();

  // Read temperature and humidity only if the sensor is present
  if (sensorPresent) {
    float temperature = aht.readTemperature();
    float humidity = aht.readHumidity();

    // Print values to Serial Monitor
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    // Send readings to Blynk
    Blynk.virtualWrite(V5, humidity); // Sending humidity to Virtual Pin V5
    Blynk.virtualWrite(V6, temperature); // Sending temperature to Virtual Pin V6
  }

  Serial.println("Blinking LED on"); // Debug statement
  digitalWrite(ledPin, LOW); // Turn the LED on
  delay(300);
  Serial.println("Blinking LED off"); // Debug statement
  digitalWrite(ledPin, HIGH); // Turn the LED off
  delay(1500);

  // Send Wi-Fi status to Bubble app
  sendWiFiStatus();
}

// Function to control the relay with a momentary pulse
BLYNK_WRITE(V1) {
  int relayState = param.asInt(); // Get the state of the button widget
  Serial.print("Relay state: ");
  Serial.println(relayState); // Print the state to Serial Monitor

  if (relayState == 1) {
    // Activate the relay (low-active)
    digitalWrite(relayPin, LOW);
    delay(PULSE_DURATION); // Keep the relay activated for the pulse duration
    // Deactivate the relay
    digitalWrite(relayPin, HIGH);
  }
}

// Function to send Wi-Fi status to Bubble app
void sendWiFiStatus() {
  HTTPClient http;
  WiFiClient wifiClient; // Create a WiFi client instance
  String url = "https://igateview.bubbleapps.io/api/1.1/wf/wifi_status"; // Bubble API endpoint URL
  String payload;

  if (WiFi.status() == WL_CONNECTED) {
    payload = "{\"status\":\"connected\"}";
    Serial.println("Wi-Fi Connected");
  } else {
    payload = "{\"status\":\"not_connected\"}";
    Serial.println("Wi-Fi Not Connected");
  }

  // Send HTTP POST request to Bubble API
  http.begin(wifiClient, url); // Use the new API format
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

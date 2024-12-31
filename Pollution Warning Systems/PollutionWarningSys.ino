//define blynk project you are on
#define BLYNK_TEMPLATE_ID "BLYNK-TEMPLATE-ID" 
#define BLYNK_TEMPLATE_NAME "BLYNK-TEMPLATE-NAME" 
#define BLYNK_AUTH_TOKEN "BLYNK-AUTH-TOKEN"

#include <WiFi.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <MHZ19.h>
#include <HardwareSerial.h>

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial

// Replace with your network credentials
const char* ssid = "YOUR-WIFI-NAME"; 
const char* password = "YOUR-WIFI-PASSWORD"; 

// Telegram Bot Token
#define CHAT_ID "TELEGRAM-ID" // Telegram ID
#define BOTtoken "TELEGRAM-BOT-CREDENTIAL" //BOT Credential

// Telegram Bot setup
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Pins for MH-Z19
#define MHZ19_RX_PIN 16
#define MHZ19_TX_PIN 17

// MH-Z19 Serial Communication
HardwareSerial mhzSerial(2);
MHZ19 mhz(&mhzSerial);
int co2;

// Pins for PM 2.5
#define measurePin 35
#define ledPower 4

int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
float voMeasured, calcVoltage, dustDensity;

void readCO2(){
  co2= mhz.getCO2();

  MHZ19_RESULT response = mhz.retrieveData();
  if (response == MHZ19_RESULT_OK)
  {
    Serial.print(F("CO2: "));
    Serial.println(co2);
    Blynk.virtualWrite(V0, co2);
  }
  else
  {
    Serial.print(F("Error, code: "));
    Serial.println(response);
  }
}

void readDust(){
  digitalWrite(ledPower, LOW);  // Turn on the LED
  delayMicroseconds(samplingTime);
  voMeasured = analogRead(measurePin);  // Read dust value
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower, HIGH);  // Turn off the LED

  calcVoltage = voMeasured * (3.3 / 4095.0);  // Map 0-3.3V to 0-4095 integer values and recover voltage
  dustDensity = 170 * calcVoltage - 0.1;

  Serial.print("Dust Density: ");
  Serial.print(dustDensity);
  Serial.println(" ug/m3");  // Output in ug/m3

  Blynk.virtualWrite(V1, dustDensity);
}

// Function to handle Telegram messages
void handleTelegramMessages() {
    if (dustDensity > 55.5){
      Serial.println("Sending Message About Dust");
      String statusMessage = "Dust: " + String(dustDensity) + " ug/m3";
      statusMessage += "\n Kandungan Debu dalam udara tinggi, mohon gunakan masker";
      bot.sendMessage(CHAT_ID, statusMessage, "Markdown");
    }

    if (co2 > 1000){
    Serial.println("Sending Message About CO2");
      String statusMessage = "CO2: " + String(co2) + " ppm";
      statusMessage += "\n Kandungan CO2 dalam udara tinggi, mohon gunakan masker";
      bot.sendMessage(CHAT_ID, statusMessage, "Markdown");
    }
}

// Setup function
void setup() {
  Serial.begin(115200);
  Serial.println("Starting Dust Sensor...");
  pinMode(ledPower, OUTPUT);
  pinMode(measurePin, INPUT);

  Serial.println("Starting CO2 Sensor...");
  mhzSerial.begin(9600, SERIAL_8N1, MHZ19_RX_PIN, MHZ19_TX_PIN);
  mhz.calibrateZero();

  // Initialize WiFi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected to: ");
  Serial.println(WiFi.localIP());

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
}

void loop() {
  readCO2();
  readDust();

  handleTelegramMessages();

  Blynk.run();
  delay(5000); // Delay for sensor stabilization
}

This is an experimental-based arduino project using ESP32 board series

Library Used:
1. WiFi.h
2. WebServer.h
3. Adafruit_Fingerprint.h
5. SPIFFS.h
6. UniversalTelegramBot.h
7. WiFiClientSecure.h

Pin Used:
// Fingerprint sensor pins
FINGERPRINT_RX_PIN 16  // Connect to TX of fingerprint sensor
FINGERPRINT_TX_PIN 17  // Connect to RX of fingerprint sensor

// Physical button pins
CHECK_IN_BUTTON_PIN 32  // GPIO for the check-in button
CHECK_OUT_BUTTON_PIN 33 // GPIO for the check-out button

This project using an ESP32 board series that need the user to connect with the same Wi-Fi as the board, 
this project also use telegram bot to send the message of the one who check-in/out,
Use Serial1 for ESP8266, and Serial2 for ESP32

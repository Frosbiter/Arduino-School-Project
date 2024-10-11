#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_Fingerprint.h>
#include <SPIFFS.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

// Replace with your network credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Telegram bot credentials
const String BOT_TOKEN = "your_bot_token";  // Replace with your Telegram Bot token
const String CHAT_ID = "your_chat_id";      // Replace with your Telegram chat ID

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Create a WebServer object on port 80
WebServer server(80);

// Fingerprint sensor pins
#define FINGERPRINT_RX_PIN 16  // Connect to TX of fingerprint sensor
#define FINGERPRINT_TX_PIN 17  // Connect to RX of fingerprint sensor

// Physical button pins
#define CHECK_IN_BUTTON_PIN 32  // GPIO for the check-in button
#define CHECK_OUT_BUTTON_PIN 33 // GPIO for the check-out button

// Initialize fingerprint sensor
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1); // Using Serial1 for fingerprint

// Global variables
String headerMessage = "Enroll Fingerprint";
const int maxUsers = 10; // Maximum number of users
String userNames[maxUsers]; // Array to store user names
String userDivisions[maxUsers]; // Array to store user divisions
bool userStatus[maxUsers]; // Array to store check-in status (true = checked in, false = checked out)
int userCount = 0; // Current user count
int enrollmentAttempts = 0; // Variable to track enrollment attempts

// Function to find an empty ID
int findEmptyID() {
  for (int id = 0; id < maxUsers; id++) {
    if (finger.loadModel(id) != FINGERPRINT_OK) {
      return id; // Return the first empty ID found
    }
  }
  return -1; // No empty IDs available
}

// Function to load user data from CSV
void loadUserData() {
  File file = SPIFFS.open("/userdata.csv", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    int commaIndex1 = line.indexOf(',');
    int commaIndex2 = line.indexOf(',', commaIndex1 + 1);
    int commaIndex3 = line.indexOf(',', commaIndex2 + 1);
    if (commaIndex1 > 0 && commaIndex2 > commaIndex1 && commaIndex3 > commaIndex2) {
      String name = line.substring(0, commaIndex1);
      String division = line.substring(commaIndex1 + 1, commaIndex2);
      bool status = line.substring(commaIndex2 + 1, commaIndex3) == "1"; // Status (1 = check-in, 0 = check-out)
      userNames[userCount] = name;
      userDivisions[userCount] = division;
      userStatus[userCount] = status;
      userCount++;
      if (userCount >= maxUsers) {
        break; // Stop if max user limit reached
      }
    }
  }
  file.close();
}

// Function to save user data to CSV
void saveUserData() {
  File file = SPIFFS.open("/userdata.csv", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  for (int i = 0; i < userCount; i++) {
    file.println(userNames[i] + "," + userDivisions[i] + "," + String(userStatus[i]));
  }
  file.close();
}

// Function to send status updates to Telegram
void sendTelegramNotification(String name, String status) {
  String message = "User: " + name + "\nStatus: " + status;
  bot.sendMessage(CHAT_ID, message, "");
  Serial.println("Telegram notification sent: " + message);
}

// HTML content for the local server
String HTMLContent() {
  String html = "<!DOCTYPE html>"
                "<html lang='en'>"
                "<head>"
                "<meta charset='UTF-8'>"
                "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                "<title>Presensi Karyawan</title>"
                "<style>"
                "body {font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #f4f4f9;}"
                "h1 {text-align: center; color: #333; padding: 20px 0;}"
                "h2 {text-align: center; color: #4CAF50; padding: 10px 0;}"
                "table {width: 60%; margin: 20px auto; border-collapse: collapse;}"
                "table, th, td {border: 1px solid #ccc;}"
                "th, td {padding: 10px; text-align: left;}"
                "th {background-color: #4CAF50; color: white;}"
                "tr:nth-child(even) {background-color: #f2f2f2;}"
                "form {text-align: center; margin-top: 20px;}"
                "input[type=text] {padding: 10px; width: 30%; margin: 5px;}"
                "input[type=submit] {padding: 10px 20px; background-color: #4CAF50; color: white; border: none; cursor: pointer;}"
                "input[type=submit]:hover {background-color: #45a049;}"
                ".circle {width: 20px; height: 20px; border-radius: 50%; display: inline-block;}"
                ".check-in {background-color: green;}"
                ".check-out {background-color: red;}"
                ".status {float: right; width: 30%; margin-right: 20px; padding: 20px; border: 1px solid #ccc; background-color: #fff;}"
                "</style>"
                "</head>"
                "<body>"
                "<h1>Presensi Karyawan</h1>"
                "<h2>" + headerMessage + "</h2>"
                "<form action='/enroll' method='post'>"
                "<input type='text' name='name' placeholder='Nama' required>"
                "<input type='text' name='division' placeholder='Divisi' required>"
                "<input type='submit' value='Enroll Fingerprint'>"
                "</form>"
                "<div class='status'>"
                "<h3>Enrollment Status</h3>"
                "<p>Attempts: " + String(enrollmentAttempts) + "</p>"
                "</div>"
                "<table>"
                "<tr><th>No</th><th>Nama</th><th>Divisi</th><th>Status Presensi</th></tr>";

  // Add enrolled user data to the table
  for (int i = 0; i < userCount; i++) {
    String statusCircle = userStatus[i] ? "<span class='circle check-in'></span>" : "<span class='circle check-out'></span>";
    html += "<tr><td>" + String(i + 1) + "</td><td>" + userNames[i] + "</td><td>" + userDivisions[i] + "</td><td>" + statusCircle + "</td></tr>";
  }

  html += "</table>"
          "</body>"
          "</html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", HTMLContent());
}

void handleEnroll() {
  if (server.arg("name") != "" && server.arg("division") != "") {
    String name = server.arg("name");
    String division = server.arg("division");
    headerMessage = "Enroll " + name + "'s Fingerprint";

    // Enroll fingerprint
    enrollFingerprint(name, division);
  }
  server.send(200, "text/html", HTMLContent());
}

// Function to enroll a fingerprint
void enrollFingerprint(String name, String division) {
  enrollmentAttempts = 0; // Reset attempts count
  // Enroll fingerprint logic
  int id = findEmptyID(); // Get an empty ID
  if (id == -1) {
    Serial.println("Not enough space to store a fingerprint.");
    return;
  }

  // Capture the first image
  Serial.println("Place your finger on the sensor...");
  int attempts = 0;
  while (finger.getImage() != FINGERPRINT_OK && attempts < 5) {
    attempts++;
    enrollmentAttempts++; // Increment attempt count
    Serial.println("Failed to capture first image. Try again.");
    delay(1000); // Wait before retrying
  }
  
  if (attempts == 5) {
    Serial.println("Failed to capture first image after multiple attempts.");
    return;
  }
  
  Serial.println("First image taken.");
  
  if (finger.image2Tz(1) != FINGERPRINT_OK) {
    Serial.println("First image conversion failed.");
    return;
  }

  // Capture the second image
  Serial.println("Place your finger on the sensor again...");
  attempts = 0;
  while (finger.getImage() != FINGERPRINT_OK && attempts < 5) {
    attempts++;
    enrollmentAttempts++; // Increment attempt count
    Serial.println("Failed to capture second image. Try again.");
    delay(1000);
  }
  
  if (attempts == 5) {
    Serial.println("Failed to capture second image after multiple attempts.");
    return;
  }
  
  Serial.println("Second image taken.");
  
  if (finger.image2Tz(2) != FINGERPRINT_OK) {
    Serial.println("Second image conversion failed.");
    return;
  }
  
  if (finger.createModel() != FINGERPRINT_OK) {
    Serial.println("Failed to create fingerprint model.");
    return;
  }

  if (finger.storeModel(id) != FINGERPRINT_OK) {
    Serial.println("Failed to store fingerprint model.");
    return;
  }

  // Add user to the list
  userNames[userCount] = name;
  userDivisions[userCount] = division;
  userStatus[userCount] = false; // Default to checked-out status
  userCount++;

  // Save the data to CSV
  saveUserData();

  // Send a Telegram notification
  sendTelegramNotification(name, "Fingerprint enrolled");

  Serial.println("Fingerprint enrolled successfully.");
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(57600, SERIAL_8N1, FINGERPRINT_RX_PIN, FINGERPRINT_TX_PIN); // Begin Serial1 for fingerprint communication

  // Initialize buttons
  pinMode(CHECK_IN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHECK_OUT_BUTTON_PIN, INPUT_PULLUP);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  // Load user data from CSV
  loadUserData();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi.");
  Serial.println(WiFi.localIP());

  // Connect to Telegram server
  client.setInsecure();

  // Initialize fingerprint sensor
  finger.begin(57600);

  // Check if the fingerprint sensor is found and responding
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor detected!");
  } else {
    Serial.println("Fingerprint sensor not detected :(");
    while (1); // Halt the program if no sensor is found
  }

  // Define routes
  server.on("/", handleRoot);
  server.on("/enroll", HTTP_POST, handleEnroll);

  // Start the server
  server.begin();
}

void loop() {
  // Handle client requests
  server.handleClient();

  // Check the button states
  if (digitalRead(CHECK_IN_BUTTON_PIN) == LOW) {
    Serial.println("Check-In button pressed.");
    updateCheckInStatus(true); // Update to check-in status
    delay(500); // Debounce delay
  }

  if (digitalRead(CHECK_OUT_BUTTON_PIN) == LOW) {
    Serial.println("Check-Out button pressed.");
    updateCheckInStatus(false); // Update to check-out status
    delay(500); // Debounce delay
  }
}

// Function to update check-in status
void updateCheckInStatus(bool isCheckedIn) {
  // Capture the fingerprint
  int attempts = 0;
  while (finger.getImage() != FINGERPRINT_OK && attempts < 5) {
    attempts++;
    Serial.println("Failed to capture fingerprint. Try again.");
    delay(1000); // Wait before retrying
  }
  
  if (attempts == 5) {
    Serial.println("Failed to capture fingerprint after multiple attempts.");
    return;
  }

  Serial.println("Fingerprint captured.");

  // Search for the captured fingerprint
  int result = finger.fingerFastSearch();
  if (result != FINGERPRINT_OK) {
    Serial.println("Fingerprint not recognized.");
    return;
  }

  // Update the user status based on fingerprint ID
  int userId = finger.fingerID; // Get the matched user ID
  userStatus[userId] = isCheckedIn;
  saveUserData(); // Save the updated status to CSV

  // Send Telegram notification
  String statusText = isCheckedIn ? "Check-In" : "Check-Out";
  sendTelegramNotification(userNames[userId], statusText);
}


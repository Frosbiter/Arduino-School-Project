#include <Adafruit_Fingerprint.h>

#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <Adafruit_Fingerprint.h>

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
HardwareSerial mySerial(2);
#else
#define mySerial Serial2
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id;
const int chipSelect = 5; //CS (Chip-Select) of SD Card module
File myFile;
File root, file;

// Function to save fingerprint template data to SD card
void saveFingerprintTemplateToSD(int id) {
    if (finger.getImage() == FINGERPRINT_OK) {
        Serial.println("Image captured.");
        if (finger.image2Tz(1) == FINGERPRINT_OK) {
            Serial.println("Image converted to template.");
            uint8_t fingerTemplate[512]; // Buffer for the template
            if (finger.fingerFastSearch() == FINGERPRINT_OK) {
                Serial.println("Fingerprint matched.");
            } else {
                Serial.println("No match found.");
            }
            // Save the template to SD card
            saveTemplateToSD(fingerTemplate, id);
        }
    } else {
        Serial.println("Failed to capture image.");
    }
}// somehow it's work

void saveTemplateToSD(uint8_t *templateData, int id) {
    File file = SD.open(String(id) + ".dat", FILE_WRITE);
    if (file) {
        file.write(templateData, 512); // Write the template data
        file.close();
        Serial.println("Template saved to SD card.");
    } else {
        Serial.println("Failed to open file for writing.");
    }
}

bool captureAndCompareFingerprint() {
    // Capture the fingerprint image
    if (finger.getImage() != FINGERPRINT_OK) {
        Serial.println("Failed to capture fingerprint.");
        return false;
    }
    
    // Convert the image to a template
    if (finger.image2Tz() != FINGERPRINT_OK) {
        Serial.println("Failed to convert image to template.");
        return false;
    }

    // Search for the captured fingerprint in the sensor's memory
    int match = finger.fingerFastSearch();
    if (match == FINGERPRINT_OK) {
        Serial.println("Fingerprint matched in internal memory!");
        return true;
    }

    // Read all .dat files from the SD card
    root = SD.open("/");
    file = root.openNextFile();
    while (file) {
        if (!file.isDirectory() && String(file.name()).endsWith(".dat")) {
            Serial.print("Checking file: ");
            Serial.println(file.name());

            // Load the fingerprint data from the file
            uint8_t templateData[512]; // Adjust size based on your template size
            size_t bytesRead = file.read(templateData, sizeof(templateData));

            if (bytesRead > 0) {
                // Compare the captured template to the loaded template
                if (compareTemplates(finger.image2Tz(), templateData)) {
                    file.close();
                    return true; // Match found
                }
            }
        }
        file.close();
        file = root.openNextFile();
        //break;
    }
    return false; // No match found
}

bool compareTemplates(unsigned char a, uint8_t *storedTemplate) {
    // Implement your comparison logic here
    // This could involve comparing byte by byte or using a hashing method
    for (int i = 0; i < 512; i++) { // Replace TEMPLATE_SIZE with the actual size of your templates
        if (a != storedTemplate[i]) {
            return false; // Templates do not match
        }
    }
    return true; // Templates match
}

void setup(){
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit Fingerprint sensor enrollment");

  // set the data rate for the sensor serial port
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }
  Serial.println("SD card initialized.");
}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop()                     // run over and over again
{
  Serial.println("Ready to enroll a fingerprint!");
  Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
  id = readnumber();
  if (id == 0) {// ID #0 not allowed, try again!
     return;
  }
  Serial.print("Enrolling ID #");
  Serial.println(id);

  while (! getFingerprintEnroll() );

  // Capture and compare fingerprints
  /*if (captureAndCompareFingerprint()) {
      Serial.println("Fingerprint matched!");
              
      Serial.printf("Your id: %d", finger.fingerID);
      Serial.println("");
  } else {
      Serial.println("No match found.");
  }
  delay(3000);*/
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return 255;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return 255;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("Could not find fingerprint features");
      return 255;
    case FINGERPRINT_INVALIDIMAGE:
      //Serial.println("Could not find fingerprint features");
      return 255;
    default:
      //Serial.println("Unknown error");
      return 255;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("Image too messy");
      return 255;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return 255;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return 255;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return 255;
    default:
      Serial.println("Unknown error");
      return 255;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return 255;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    //Serial.println("Fingerprints did not match");
    return 255;
  } else {
    //Serial.println("Unknown error");
    return 255;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");

    saveFingerprintTemplateToSD(id);
    Serial.println("Stored to SD Card");

  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return 255;
  } else if (p == FINGERPRINT_BADLOCATION) {
    //Serial.println("Could not store in that location");
    return 255;
  } else if (p == FINGERPRINT_FLASHERR) {
    //Serial.println("Error writing to flash");
    return 255;
  } else {
    //Serial.println("Unknown error");
    return 255;
  }

  return true;
}

/*
1. mencari total data yang ada di SD Card
2. for loop mencocokkan isi file di SD card dengan finger yang didaftarkan (dat to finger), break jika sama dengan finger yang sekarang
3. menggunakan sub-program return, apabila tidak ada jari yang sama maka return -1
*/

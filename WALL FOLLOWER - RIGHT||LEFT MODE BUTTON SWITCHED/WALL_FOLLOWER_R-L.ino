//This Project is using ESP8266, so many pin might not the same with the microcontroller you currently use

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define pins for ultrasonic sensor
#define FTRIG  0 // D3
#define FECHO  16 // D0

#define LTRIG 14 // D5
#define LECHO 2 // D4

#define RTRIG 1 // TX
#define RECHO 3 // RX

// Define pins for motor control
#define LEFT_MOTOR_FORWARD 13 // D7
#define RIGHT_MOTOR_FORWARD 15 // D8

// define pins for Button
#define MODE_BUTTON 12 //D6

int count = 0;
int L_Dist, F_Dist, R_Dist;
float distance;
double outPID;

//PID
int sp = 10; //setpoint, jarak dengan dinding
double error, sumError, lastError;
double deltaError;
double basePWM = 255;
double Kp = 6.5;
double Ki = 0.5;
double Kd = 0.6;
double Ts = 1;
double P;
double I;
double D;
double R_Motor;
double L_Motor;

//Batas Kecepatan 
double MAX_SPEED = 230;
double MIN_SPEED = 120;

// Initialize the LCD library with the I2C address and the LCD size
LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust the address 0x27 to match your I2C LCD module

/*~~~~~~~~~~~SENSOR READ~~~~~~~~~~~~~~*/
float sensorRead(int echo_pin, int trig_pin){
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);
  
  long echotime= pulseIn(echo_pin, HIGH);
  distance = 0.0001*((float)echotime*340.0)/2.0;
  //Serial.print(distance);
  //Serial.println(" cm"); 
  if(distance > 30){
    distance = 30;
    }else {
      distance = distance;
      }
  

  return distance;
  }
/*~~~~~~~~~END SENSOR READ~~~~~~~~~~~*/

/*~~~~~~~~~~~~~~MOVEMENT CODE~~~~~~~~~~~~~~~~*/
void wallFollowing(int sideDist){
  error = sp - sideDist;
  deltaError = error - lastError;
  sumError += lastError;
  P = Kp * error;
  I = Ki * sumError;
  D = (Kd/Ts) * deltaError;
  lastError = error;
  outPID = P+I+D;
  L_Motor = basePWM - outPID;
  R_Motor = basePWM + outPID; 
  /*apakah ini hanya berfungsi untuk leftWall following? jika iya + dan - 
  diubah untuk right wall follower, gunakan if count...*/

  if (L_Motor > MAX_SPEED)L_Motor = MAX_SPEED;
  if (L_Motor < MIN_SPEED)L_Motor = MIN_SPEED;
  if (R_Motor > MAX_SPEED)R_Motor = MAX_SPEED;
  if (R_Motor < MIN_SPEED)R_Motor = MIN_SPEED;

  analogWrite(LEFT_MOTOR_FORWARD, L_Motor);
  analogWrite(RIGHT_MOTOR_FORWARD, R_Motor);
}
/*~~~~~~~~END MOVEMENT~~~~~~~~*/

/*~~~~~~~~~~~~~~SETUP ENVIRONEMENT~~~~~~~~~~~~~~~*/
void setup() {
  // Initialize the serial communication
  Serial.begin(9600);

  // Set trigger pin as output and echo pin as input
  pinMode(FTRIG, OUTPUT);
  pinMode(FECHO, INPUT);
  pinMode(LTRIG, OUTPUT);
  pinMode(LECHO, INPUT);
  pinMode(RTRIG, OUTPUT);
  pinMode(RECHO, INPUT);

  // Set motor control pins as output
  pinMode(LEFT_MOTOR_FORWARD, OUTPUT);
  pinMode(RIGHT_MOTOR_FORWARD, OUTPUT);

  //set button mode as input
  pinMode(MODE_BUTTON, INPUT);

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
}
/*~~~~~~~~~~~~~~END SETUP~~~~~~~~~~~~~~~*/

/*~~~~~~~~~~~~~~RUNNING CODE~~~~~~~~~~~~~~~*/
void loop() {
  //reading the button
  int buttonState = digitalRead(MODE_BUTTON);
  
  Serial.println(buttonState);

  if (buttonState==HIGH){
    count = count+1;
  }

  // Calculate the distance in cm
  L_Dist = sensorRead(LTRIG, LECHO);
  F_Dist = sensorRead(FTRIG, FECHO);
  R_Dist = sensorRead(RTRIG, RECHO);

  // Print the distance to the serial monitor
  Serial.print("F_Dist : ");
  Serial.print(F_Dist);
  Serial.println(" cm");

  Serial.print("R_Dist : ");
  Serial.print(R_Dist);
  Serial.println(" cm");

  Serial.print("L_Dist : ");
  Serial.print(L_Dist);
  Serial.println(" cm");

  // Display the distance on the LCD
  lcd.clear();
  lcd.setCursor(0, 0);  // Set cursor to column 0, row 0
  lcd.print("L");
  lcd.setCursor(1, 0);  // Set cursor to column 1, row 0
  lcd.print(":");
  lcd.setCursor(2, 0);  // Set cursor to column 2, row 0
  lcd.print(L_Dist);

  lcd.setCursor(6, 0);  // Set cursor to column 6, row 0
  lcd.print("F");
  lcd.setCursor(7, 0);  // Set cursor to column 7, row 0
  lcd.print(":");
  lcd.setCursor(8, 0);  // Set cursor to column 8, row 0
  lcd.print(F_Dist);

  lcd.setCursor(12, 0);  // Set cursor to column 12, row 0
  lcd.print("R");
  lcd.setCursor(13, 0);  // Set cursor to column 13, row 0
  lcd.print(":");
  lcd.setCursor(14, 0);  // Set cursor to column 14, row 0
  lcd.print(R_Dist);

  switch(count){
    case 0:
    //STOP
      lcd.setCursor(0,1);
      lcd.print("MODE: ");
      lcd.setCursor(6, 1);
      lcd.print("S");
      lcd.setCursor(11, 1);
      lcd.print("GASKE");

      digitalWrite(LEFT_MOTOR_FORWARD, 0);
      digitalWrite(LEFT_MOTOR_FORWARD, 0);
      break;

    case 1:
      //left wall follower
      lcd.setCursor(0,1);
      lcd.print("MODE: ");
      lcd.setCursor(6, 1);
      lcd.print("L");
      lcd.setCursor(11, 1);
      lcd.print("GASKE");

      if(F_Dist <= 9){
        analogWrite(LEFT_MOTOR_FORWARD, 0);
        analogWrite(RIGHT_MOTOR_FORWARD, 230);
      }else if (L_Dist >= 15){
        analogWrite(LEFT_MOTOR_FORWARD, 150);
        analogWrite(RIGHT_MOTOR_FORWARD, 80);
      }else{
        wallFollowing(L_Dist);
      }
      break;

    case 2:
      // right wall follower
      lcd.setCursor(0,1);
      lcd.print("MODE: ");
      lcd.setCursor(6, 1);
      lcd.print("R");
      lcd.setCursor(11, 1);
      lcd.print("GASKE");

      if(F_Dist <= 9){
        analogWrite(LEFT_MOTOR_FORWARD, 230);
        analogWrite(RIGHT_MOTOR_FORWARD, 0);
      }else if (R_Dist >= 15){
        analogWrite(LEFT_MOTOR_FORWARD, 80);
        analogWrite(RIGHT_MOTOR_FORWARD, 150);
      }else{
        wallFollowing(R_Dist);
      }
      break;

      default:
        analogWrite(LEFT_MOTOR_FORWARD, 0);
        analogWrite(RIGHT_MOTOR_FORWARD, 0);

        if (count>2){
          count = 0;
        }
        break;
  }

  delay(200);
}

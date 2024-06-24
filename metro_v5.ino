/*
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 10
*/

//SD card libraries
#include <SPI.h>
#include <SD.h>
//MPU-6050 libraries
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

const int switchPin = 2;
const int ledRed = 6;
const int ledGreen = 7;
const int ledBlue = 8;
const int chipSelect = 10; //SD CS pin on Arduino

//calibration data for the accelerometer, calibrated when button is pressed
float calX = 9.769;
float calY = 0.573;
float calZ = -1.311;
int calTimes = 10;
int debounce = 10;//time necessary to read_toggle_switch()

Adafruit_MPU6050 mpu;

String fileNumber = "";
String fileName = "";
bool swicthStatus = LOW; //arduino starts off
bool isCollecting = false;

bool read_toggle_switch() { //code we need for thw swicth bc of transition times
  static long int elapse_time = 0;
  static bool transition_started = false;
  int pin_value;
  pin_value = digitalRead(switchPin);
  pin_value = !pin_value;
  if (pin_value  != swicthStatus && !transition_started) {
    transition_started = true;
    elapse_time = millis(); 
  } else {
    if (transition_started) {
      if (millis() - elapse_time >= debounce) {
        swicthStatus  = !swicthStatus;  
        transition_started = false;  
      }
    }
  }
  return swicthStatus;
}

void ledColor(int r, int g, int b) {
  if (r == 1) { //for errors or off
    digitalWrite(ledRed, HIGH);
  } else {
    digitalWrite(ledRed, LOW);
  }
  if (g == 1) { //when its creating a new file
    digitalWrite(ledGreen, HIGH);
  } else {
    digitalWrite(ledGreen, LOW);
  }
  if (b == 1) { //when its not collecting?
    digitalWrite(ledBlue, HIGH);
  } else {
    digitalWrite(ledBlue, LOW);
  }
}

void setup(void) {
  //white light indicating active board
  ledColor(1,1,1);

  pinMode(switchPin,  INPUT_PULLUP);

  Serial.begin(115200);
  while (!Serial) {
    delay(10); // will pause Zero, Leonardo, etc until serial console opens
  }

  // Try to initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    ledColor(1,0,0);
    while (1) {
      delay(10);
    }
  }

  //Try to initialize SD card
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    ledColor(1,0,0);
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");
  for (int i=0; i<5; i++) { //blink x5 green
    ledColor(0,1,0);
    delay(100);
    ledColor(0,0,0);
    delay(100);
  }
  //MPU 6050 setup
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G); //the smallest possible
  mpu.setGyroRange(MPU6050_RANGE_250_DEG); //useless
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); //no idea what that does
  Serial.println("");

  ledColor(0,0,1);
  delay(100);

}

int createNextFile() {
  int i = 0;
  String number = "";
  do {
    i += 1;
    number = String(i);
    fileName = "metro" + number + ".csv";
  } while (SD.exists(fileName));  //checks for numbered files and increments i when it does. breaks when the file does not already exist.
  Serial.println("The file will be named: " + fileName);

  for (int i=0; i<5; i++) { //blink x5 green
    ledColor(0,1,0);
    delay(100);
    ledColor(0,0,0);
    delay(100);
  }
  return i;
}

/*
void calibrate() {
  Serial.println("Calibrating the accelerometer...");
  float sumX = 0.0;
  float sumY = 0.0;
  float sumZ = 0.0;
  for (int i = 0; i < calTimes; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    sumX += a.acceleration.x;
    sumY += a.acceleration.y;
    sumZ += a.acceleration.z;
  }
  calX = (sumX / calTimes) - 9.80;
  calY = sumY / calTimes;
  calZ = sumZ / calTimes;
  Serial.println("Calibration complete!");
}
*/

void loop() {
  if (read_toggle_switch() == LOW) { //if its off
    isCollecting = false;
    ledColor(0,0,1);
  }
  else { //start collecting data
    isCollecting = true;
    fileNumber = String(createNextFile());
  //calibrate();
  }

  if (isCollecting) {
    // make a string for assembling the data to log:
    String dataString = "";

    /* Get new sensor events with the readings */
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
/*
    // Print out the values FOR TESTING: 
    Serial.print("aX:");
    Serial.print(a.acceleration.x - calX);
    Serial.print(",");
    Serial.print("aY:");
    Serial.print(a.acceleration.y - calY);
    Serial.print(",");
    Serial.print("aZ:");
    Serial.print(a.acceleration.z - calZ);
    Serial.print(", ");
    Serial.println("");
*/

    //Write data to SD
    dataString += String(a.acceleration.x - calX);
    dataString += ",";
    dataString += String(a.acceleration.y - calY);
    dataString += ",";
    dataString += String(a.acceleration.z - calZ);
    dataString += ",";

    File dataFile = SD.open(fileName, FILE_WRITE);

    // if the file is available, write to it:
    if (dataFile) {
      ledColor(0,1,0);
      dataFile.println(dataString);
      dataFile.close();
    }
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening " + fileName);
      ledColor(1,0,0);
    }
  }

  delay(100);
}
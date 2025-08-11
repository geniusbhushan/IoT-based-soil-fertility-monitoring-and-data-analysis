#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AltSoftSerial.h>
#include <Ethernet.h>

#include <LiquidCrystal_I2C.h>
#define RE 6
#define DE 7

LiquidCrystal_I2C lcd(0x27, 20, 4);  // Address 0x27, 20 columns, 4 rows
const int buttonPin1 = 2;  // Pin for changing crop name
const int buttonPin2 = 3;  // Pin for selecting crop
const int buttonPin3 = 4;  // Pin for live sensor data display

const byte temp[] = {0x01,0x03, 0x00, 0x13, 0x00, 0x01, 0x75, 0xcf};
const byte mois[]  = {0x01,0x03,0x00,0x12,0x00,0x01,0x24,0x0F};
const byte econ[] = {0x01,0x03, 0x00, 0x15, 0x00, 0x01, 0x95, 0xce};
const byte ph[] = {0x01,0x03, 0x00, 0x06, 0x00, 0x01, 0x64, 0x0b};
const byte nitro[] = { 0x01, 0x03, 0x00, 0x1E, 0x00, 0x01, 0xE4, 0x0C };
const byte phos[] = { 0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc };
const byte pota[] = { 0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0 };

byte values[11];
AltSoftSerial mod;

// Define the names of crops
String crops[] = {"Tomato", "Wheat", "Potato", "Lettuce", "Corn", "Sugar Cane", "Rice", "Jowar", "Bajra", "Mung"};
int currentCropIndex = 0;

// Define the NPK recommendations for each crop
float npkRecommendations[][3] = {
  {150.0, 75.0, 175.0},  // Tomato
  {200.0, 45.0, 60.0},   // Wheat
  {115.0, 150.0, 115.0},   // Potato
  {35.0, 55.0, 55.0},    // Lettuce
  {200.0, 45.0, 55.0},   // Corn
  {100.0, 60.0, 160.0},   // Sugar Cane
  {150.0, 75.0, 100.0},  // Rice
  {100.0, 50.0, 80.0},   // Jowar
  {100.0, 60.0, 115.0},    // Bajra
  {35.0, 45.0, 60.0}     // Mung
};

// Define the pH recommendations for each crop
float phRecommendations[] = {6.4, 6.7, 5.7, 6.2, 6.5, 6.7, 6.2, 6.0, 6.7, 6.5};

// Define the EC recommendations for each crop
float ecRecommendations[] = {2.0, 1.5, 1.2, 0.8, 1.5, 1.5, 1.0 , 1.8, 1.0, 0.6};

// Define the temperature recommendations for each crop (in degrees Celsius)
float temperatureRecommendations[] = {21, 15, 17, 14, 24, 25, 27, 28, 30, 30};

// Define the moisture recommendations for each crop (in percentage)
float moistureRecommendations[] = {70, 60, 75, 75, 65, 77, 85, 60, 65, 65};

// Define assumed sensor values
float nitrogenValue;
float phosphorusValue;
float potassiumValue;
float pHValue;
float ecValue;
float moistu12;
float temperatureValue;
float moistureValue;
unsigned long previousMillis = 0;   // will store last time sensor values were updated
unsigned long printMillis = 0;      // will store last time sensor values were printed
const long sensorInterval = 100;    // interval at which to read sensor values (milliseconds)
const long printInterval = 1000;
unsigned long buttonPress1Time = 0;   // store last time button 1 was pressed
unsigned long buttonPress2Time = 0;
unsigned long buttonPress3Time = 0;   // store last time button 3 was pressed

float averageReadings(float (*readFunction)(), int numReadings) {
  float total = 0;
  for (int i = 0; i < numReadings; i++) {
    total += readFunction();
    delay(10);  // delay between readings
  }
  return total / numReadings;
}
void setup() {
  lcd.init();                      // Initialize LCD
  lcd.backlight(); 
  Serial.begin(9600);
  mod.begin(9600);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);                // Turn on backlight
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);   // New button for live sensor data display
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(13, OUTPUT);
  lcd.setCursor(3, 0);
  lcd.print("IOT based soil ");
  lcd.setCursor(0, 1);
  lcd.print("fertility monitoring");
  lcd.setCursor(7, 2);
  lcd.print(" system ");
  delay(2500);
  lcd.clear();
  displayCropName();
}

void loop() {

  moistureValue  = moisture()/2.32;
   
    temperatureValue = temperature() /2.3;
    ecValue = econduc()/23.7;
    pHValue = phydrogen() / 25;
    nitrogenValue = nitrogen();
    phosphorusValue = phosphorous();
    potassiumValue = potassium();

  unsigned long currentMillis = millis();  // current time

  // Task 1: Read sensor values
  if (currentMillis - previousMillis >= sensorInterval) {
    // Read sensor values
  moistureValue = moisture()/2.32;
   
    temperatureValue = temperature() /2.3;
    ecValue = econduc()/23.7;
    pHValue = phydrogen() / 25;
    nitrogenValue = nitrogen();
    phosphorusValue = phosphorous();
    potassiumValue = potassium();
  
    previousMillis = currentMillis;
  }

  // Task 2: Check and handle button presses
  int buttonState1 = digitalRead(buttonPin1);
  int buttonState2 = digitalRead(buttonPin2);
  int buttonState3 = digitalRead(buttonPin3);  // New button state

  if (buttonState1 == LOW && currentMillis - buttonPress1Time > 300) {  
    buttonPress1Time = currentMillis;
    currentCropIndex = (currentCropIndex + 1) % (sizeof(crops) / sizeof(crops[0]));
     displayCropName();  
  }

  if (buttonState2 == LOW && currentMillis - buttonPress2Time > 300) {
    buttonPress2Time = currentMillis;
    analyzeSoilAndDisplay();
  }

  if (buttonState3 == LOW && currentMillis - buttonPress3Time > 300) {
    buttonPress3Time = currentMillis;
   displayLiveSensorData();// Call function to display live sensor data
  }

  // Task 3: Print sensor values
  if (currentMillis - printMillis >= printInterval) {
    // Print sensor values
    Serial.print(moistureValue);
    Serial.print(',');
    Serial.print(temperatureValue);
    Serial.print(',');
    Serial.print(ecValue);
    Serial.print(',');
    Serial.print(pHValue);
    Serial.print(',');
    Serial.print(nitrogenValue);
    Serial.print(',');
    Serial.print(phosphorusValue);
    Serial.print(',');
    Serial.println(potassiumValue);
    
    // Update last print time
    printMillis = currentMillis;
  }
}

void displayCropName() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select Crop:");

  lcd.setCursor(0, 1);
  lcd.print(crops[currentCropIndex]);
}

void analyzeSoilAndDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Crop: ");
  lcd.print(crops[currentCropIndex]);

  lcd.setCursor(0, 1);
  lcd.print("N: ");
  if (nitrogenValue < npkRecommendations[currentCropIndex][0]) {
    lcd.print("Add ");
    lcd.print(npkRecommendations[currentCropIndex][0] - nitrogenValue);
    lcd.print(" mg/kg");
  } else {
    lcd.print("No");
  }

  lcd.setCursor(0, 2);
  lcd.print("P: ");
  if (phosphorusValue < npkRecommendations[currentCropIndex][1]) {
    lcd.print("Add ");
    lcd.print(npkRecommendations[currentCropIndex][1] - phosphorusValue);
    lcd.print(" mg/kg");
  } else {
    lcd.print("No");
  }

  lcd.setCursor(0, 3);
  lcd.print("K: ");
  if (potassiumValue < npkRecommendations[currentCropIndex][2]) {
    lcd.print("Add ");
    lcd.print(npkRecommendations[currentCropIndex][2] - potassiumValue);
    lcd.print(" mg/kg");
  } else {
    lcd.print("No");
  }
 delay(3000);
 lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("pH: ");
  if (pHValue < phRecommendations[currentCropIndex]) {
    lcd.print("Adj ");
    lcd.print(phRecommendations[currentCropIndex]);
  } else {
    lcd.print("No");
  }

  lcd.setCursor(0, 2);
  lcd.print("EC: ");
  if (ecValue < ecRecommendations[currentCropIndex]) {
    lcd.print("Adj ");
    lcd.print(ecRecommendations[currentCropIndex] - ecValue);
    lcd.print(" mS/cm");
  } else {
    lcd.print("No");
  }

  lcd.setCursor(0, 3);
  lcd.print("Temp: ");
  if (temperatureValue < temperatureRecommendations[currentCropIndex]) {
    lcd.print("Adj ");
    lcd.print(temperatureRecommendations[currentCropIndex]);
    lcd.print("C");
  } else {
    lcd.print("No");
  }
  
  lcd.setCursor(0, 1);
  lcd.print("Mois: ");
  if (moistureValue < moistureRecommendations[currentCropIndex]) {
    lcd.print("Adj ");
    lcd.print(moistureRecommendations[currentCropIndex]);
    lcd.print("%");
  } else {
    lcd.print("No");
  }
}
void displayLiveSensorData() {
  // Display live sensor data until the button is released
  while (digitalRead(buttonPin3) == LOW) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Live Sensor Data:");
    lcd.setCursor(0, 1);
    lcd.print("Moisture: ");
    lcd.print(moistureValue);
    lcd.print("%");
    lcd.setCursor(0, 2);
    lcd.print("Temperature: ");
    lcd.print(temperatureValue);
    lcd.print("C");
    lcd.setCursor(0, 3);
    lcd.print("EC: ");
    lcd.print(ecValue);
    lcd.print("mS/cm");
    delay(3000);
    lcd.clear();
    // Print NPK values
   lcd.setCursor(0, 0);
    lcd.print("pH:");
    lcd.print(pHValue);
    lcd.setCursor(0, 1);
    lcd.print("N:");
    lcd.print(nitrogenValue);
     lcd.print("mg/kg");
    lcd.setCursor(0, 2);
    lcd.print("P:");
    lcd.print(phosphorusValue);
      lcd.print("mg/kg");
     lcd.setCursor(0, 3);
    lcd.print("K:");
    lcd.print(potassiumValue);
      lcd.print("mg/kg");

    delay(1000); // Update display every second
  }
}
byte moisture() {
  // clear the receive buffer
  mod.flushInput();

  // switch RS-485 to transmit mode
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(1);

  // write out the message
  for (uint8_t i = 0; i < sizeof(mois); i++) mod.write(mois[i]);

  // wait for the transmission to complete
  mod.flush();

  // switching RS485 to receive mode
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  // delay to allow response bytes to be received!
  delay(200);

  // read in the received bytes
  for (byte i = 0; i < 7; i++) {
    values[i] = mod.read();
    // Serial.print(values[i], HEX);
    // Serial.print(' ');
  }
  return values[4];
}

byte temperature() {
  // clear the receive buffer
  mod.flushInput();

  // switch RS-485 to transmit mode
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(1);

  // write out the message
  for (uint8_t i = 0; i < sizeof(temp); i++) mod.write(temp[i]);

  // wait for the transmission to complete
  mod.flush();

  // switching RS485 to receive mode
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  // delay to allow response bytes to be received!
  delay(200);

  // read in the received bytes
  for (byte i = 0; i < 7; i++) {
    values[i] = mod.read();
    // Serial.print(values[i], HEX);
    // Serial.print(' ');
  }
  return values[3]<<8|values[4];
}

byte econduc() {
  // clear the receive buffer
  mod.flushInput();

  // switch RS-485 to transmit mode
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(1);

  // write out the message
  for (uint8_t i = 0; i < sizeof(econ); i++) mod.write(econ[i]);

  // wait for the transmission to complete
  mod.flush();

  // switching RS485 to receive mode
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  // delay to allow response bytes to be received!
  delay(200);

  // read in the received bytes
  for (byte i = 0; i < 7; i++) {
    values[i] = mod.read();
    // Serial.print(values[i], HEX);
    // Serial.print(' ');
  }
  return values[4];
}

byte phydrogen() {
  // clear the receive buffer
  mod.flushInput();
  // switch RS-485 to transmit mode
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(1);

  // write out the message
  for (uint8_t i = 0; i < sizeof(ph); i++) mod.write(ph[i]);

  // wait for the transmission to complete
  mod.flush();

  // switching RS485 to receive mode
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  // delay to allow response bytes to be received!
  delay(200);

  // read in the received bytes
  for (byte i = 0; i < 7; i++) {
    values[i] = mod.read();
    // Serial.print(values[i], HEX);
    // Serial.print(' ');
  }
  return values[4];
}

byte nitrogen() {
  // clear the receive buffer
  mod.flushInput();

  // switch RS-485 to transmit mode
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(1);

  // write out the message
  for (uint8_t i = 0; i < sizeof(nitro); i++) mod.write(nitro[i]);

  // wait for the transmission to complete
  mod.flush();

  // switching RS485 to receive mode
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  // delay to allow response bytes to be received!
  delay(200);

  // read in the received bytes
  for (byte i = 0; i < 7; i++) {
    values[i] = mod.read();
    // Serial.print(values[i], HEX);
    // Serial.print(' ');
  }
  return values[4];
}

byte phosphorous() {
  mod.flushInput();
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(1);
  for (uint8_t i = 0; i < sizeof(phos); i++) mod.write(phos[i]);
  mod.flush();
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);
  // delay to allow response bytes to be received!
  delay(200);
  for (byte i = 0; i < 7; i++) {
    values[i] = mod.read();
    // Serial.print(values[i], HEX);
    // Serial.print(' ');
  }
  return values[4];
}

byte potassium() {
  mod.flushInput();
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(1);
  for (uint8_t i = 0; i < sizeof(pota); i++) mod.write(pota[i]);
  mod.flush();
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);
  // delay to allow response bytes to be received!
  delay(200);
  for (byte i = 0; i < 7; i++) {
    values[i] = mod.read();
    // Serial.print(values[i], HEX);
    // Serial.print(' ');
  }
  return values[4];
}

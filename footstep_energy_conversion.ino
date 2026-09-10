/*
  Footstep to Energy Conversion System
  Arduino Nano with Piezoelectric Sensor
  Components: Piezo sensor, 16x2 LCD, Lithium-ion battery, BC547 transistor, 4V LED
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD Setup (16x2, I2C address 0x27 - adjust if needed: 0x3F for some modules)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Definitions
const int PIEZO_PIN = A0;          // Piezo sensor input
const int STORAGE_CAP_PIN = A1;    // Storage capacitor voltage measurement
const int LED_PIN = 9;             // PWM pin for LED (via BC547 transistor)
const int TRANSISTOR_BASE = 10;    // BC547 base control pin
const int PUSH_BUTTON = 2;         // Optional reset button

// Configuration
const int THRESHOLD = 80;          // Footstep detection threshold (adjust 50-150)
const int DEBOUNCE_TIME = 300;     // Minimum time between step detections (ms)
const int CAPACITOR_VALUE = 10;    // Capacitor in microfarads (10µF)

// Variables
unsigned long stepCount = 0;
float totalEnergyHarvested = 0;    // Energy in microjoules
float peakVoltageRecorded = 0;
float maxStoredVoltage = 0;
unsigned long lastStepTime = 0;
unsigned long sessionStartTime = 0;

void setup() {
  Serial.begin(9600);
  
  // Initialize pins
  pinMode(PIEZO_PIN, INPUT);
  pinMode(STORAGE_CAP_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TRANSISTOR_BASE, OUTPUT);
  pinMode(PUSH_BUTTON, INPUT_PULLUP);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.print("  Footstep Energy");
  lcd.setCursor(0, 1);
  lcd.print("   Converter v1");
  
  Serial.println("=== Footstep Energy Conversion System ===");
  Serial.println("Components: Piezo + LCD + Li-ion Battery + BC547 + 4V LED");
  Serial.println("Initializing...");
  
  delay(2000);
  lcd.clear();
  
  sessionStartTime = millis();
  
  // Flash LED to indicate startup
  flashLED(3, 100);
  
  displayWelcomeScreen();
  delay(2000);
}

void loop() {
  // Read sensor values
  int piezoValue = analogRead(PIEZO_PIN);
  int storageCapValue = analogRead(STORAGE_CAP_PIN);
  float storedVoltage = (storageCapValue * 5.0) / 1023.0;
  
  // Footstep Detection
  if(piezoValue > THRESHOLD && (millis() - lastStepTime) > DEBOUNCE_TIME) {
    lastStepTime = millis();
    stepCount++;
    
    float peakVoltage = (piezoValue * 5.0) / 1023.0;
    
    // Track peak voltage
    if(peakVoltage > peakVoltageRecorded) {
      peakVoltageRecorded = peakVoltage;
    }
    
    // Track maximum stored voltage
    if(storedVoltage > maxStoredVoltage) {
      maxStoredVoltage = storedVoltage;
    }
    
    // Calculate energy harvested per step
    // Energy = 0.5 * C * V²
    float energy = 0.5 * (CAPACITOR_VALUE * 1e-6) * (peakVoltage * peakVoltage);
    totalEnergyHarvested += energy;
    
    // Control LED brightness based on voltage
    int brightness = map((int)(peakVoltage * 100), 0, 500, 0, 255);
    brightness = constrain(brightness, 0, 255);
    analogWrite(LED_PIN, brightness);
    digitalWrite(TRANSISTOR_BASE, HIGH);
    
    // Serial output
    Serial.print("Step #");
    Serial.print(stepCount);
    Serial.print(" | Peak Voltage: ");
    Serial.print(peakVoltage, 2);
    Serial.print("V | Stored: ");
    Serial.print(storedVoltage, 2);
    Serial.print("V | Energy: ");
    Serial.print(totalEnergyHarvested * 1000, 2);  // Convert to nanojoules
    Serial.println(" nJ");
    
    // Update LCD display
    updateLCD(peakVoltage, storedVoltage);
    
    delay(300);  // Step detection window
    digitalWrite(TRANSISTOR_BASE, LOW);
    analogWrite(LED_PIN, 0);
  }
  
  // Check for reset button press
  if(digitalRead(PUSH_BUTTON) == LOW) {
    delay(50);  // Debounce
    if(digitalRead(PUSH_BUTTON) == LOW) {
      resetSystem();
      delay(1000);
    }
  }
  
  // Periodic display update (every 500ms)
  static unsigned long lastDisplayUpdate = 0;
  if(millis() - lastDisplayUpdate > 500) {
    lastDisplayUpdate = millis();
    updateLCDIdle(storedVoltage);
  }
  
  delay(20);
}

void updateLCD(float peakVoltage, float storedVoltage) {
  lcd.clear();
  
  // Line 1: Step count and peak voltage
  lcd.setCursor(0, 0);
  lcd.print("Step:");
  lcd.print(stepCount);
  lcd.print(" ");
  lcd.print(peakVoltage, 2);
  lcd.print("V");
  
  // Line 2: Stored voltage and energy
  lcd.setCursor(0, 1);
  lcd.print("Store:");
  lcd.print(storedVoltage, 2);
  lcd.print("V ");
  lcd.print((int)totalEnergyHarvested);
  lcd.print("uJ");
}

void updateLCDIdle(float storedVoltage) {
  static int displayMode = 0;
  
  lcd.clear();
  
  if(displayMode == 0) {
    // Display steps and time
    unsigned long elapsedTime = (millis() - sessionStartTime) / 1000;
    lcd.setCursor(0, 0);
    lcd.print("Steps: ");
    lcd.print(stepCount);
    
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    printTime(elapsedTime);
    
    displayMode = 1;
  }
  else if(displayMode == 1) {
    // Display voltage information
    lcd.setCursor(0, 0);
    lcd.print("Stored: ");
    lcd.print(storedVoltage, 2);
    lcd.print("V");
    
    lcd.setCursor(0, 1);
    lcd.print("Max: ");
    lcd.print(maxStoredVoltage, 2);
    lcd.print("V");
    
    displayMode = 2;
  }
  else {
    // Display energy harvested
    lcd.setCursor(0, 0);
    lcd.print("Total Energy:");
    
    lcd.setCursor(0, 1);
    lcd.print((long)(totalEnergyHarvested * 1000));
    lcd.print(" nJ");
    
    displayMode = 0;
  }
}

void displayWelcomeScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready to Harvest");
  lcd.setCursor(0, 1);
  lcd.print("Step on sensor!");
}

void printTime(unsigned long seconds) {
  unsigned long hours = seconds / 3600;
  unsigned long minutes = (seconds % 3600) / 60;
  unsigned long secs = seconds % 60;
  
  if(hours > 0) {
    lcd.print(hours);
    lcd.print("h ");
  }
  if(minutes > 0) {
    lcd.print(minutes);
    lcd.print("m ");
  }
  lcd.print(secs);
  lcd.print("s");
}

void flashLED(int times, int duration) {
  for(int i = 0; i < times; i++) {
    digitalWrite(TRANSISTOR_BASE, HIGH);
    analogWrite(LED_PIN, 200);
    delay(duration);
    digitalWrite(TRANSISTOR_BASE, LOW);
    analogWrite(LED_PIN, 0);
    delay(duration);
  }
}

void resetSystem() {
  Serial.println("\n=== System Reset ===");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Reset!");
  lcd.setCursor(0, 1);
  lcd.print("Clearing Data...");
  
  // Reset all variables
  stepCount = 0;
  totalEnergyHarvested = 0;
  peakVoltageRecorded = 0;
  maxStoredVoltage = 0;
  sessionStartTime = millis();
  
  flashLED(5, 100);
  
  delay(1000);
  lcd.clear();
  displayWelcomeScreen();
  
  Serial.println("Reset Complete. Ready for new session.");
}

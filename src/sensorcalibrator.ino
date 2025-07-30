#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int trigPin = 10;
const int echoPin = 11;

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  lcd.begin(16, 2, 0);
  lcd.clear();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("Sensor Calibrator");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  
  Serial.println("Simple Sensor Calibrator");
  Serial.println("Measuring distance every second...");
  
  delay(2000);
  lcd.clear();
}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  float distance = (duration * 0.034) / 2;
  
  lcd.setCursor(0, 0);
  lcd.print("Distance:       ");
  lcd.setCursor(0, 1);
  lcd.print(distance, 1);
  lcd.print(" cm     ");
  
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  delay(1000);
}
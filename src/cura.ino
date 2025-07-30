#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int TRIG_PIN = 10, ECHO_PIN = 11;

// put the data from sensorcalibrator.ino here
const float EMPTY_DISTANCE = 22.0, CHANGE_THRESHOLD = 2.0;
const int READINGS_COUNT = 12, IDLE_THRESHOLD = 3, BLINK_DURATION = 150, EMOTE_DURATION = 2000;
const long MEASUREMENT_INTERVAL = 200, IDLE_MEASUREMENT_INTERVAL = 4000;

float lastDisplayedDistance = 0;
int stablePercentage = 0, lastPercentage = -1, samePercentageCount = 0, currentEmote = 0, lastEmoteType = -1, lastDisplayedPercentage = -1;
bool isIdle = false, isBlinking = false, isSpecialEmote = false, lastBlinkState = false, needsRedraw = true;
unsigned long lastMeasurementTime = 0, lastBlinkTime = 0, nextBlinkDelay = 0, blinkStartTime = 0, lastEmoteTime = 0, nextEmoteDelay = 0, emoteStartTime = 0;

byte openEye[8] = {B00000,B01110,B11111,B11011,B11111,B01110,B00000,B00000};
byte closedEye[8] = {B00000,B00000,B01110,B11111,B01110,B00000,B00000,B00000};
byte neutralMouth[8] = {B00000,B00000,B00000,B01110,B01110,B00000,B00000,B00000};
byte smileMouth[8] = {B00000,B00000,B00000,B10001,B01110,B00000,B00000,B00000};
byte uwuMouth[8] = {B00000,B00000,B00000,B01010,B00100,B00000,B00000,B00000};
byte worriedMouth[8] = {B00000,B00000,B00000,B01110,B10001,B00000,B00000,B00000};
byte happyEye[8] = {B00000,B00000,B01010,B01110,B01010,B00000,B00000,B00000};
byte worriedEye[8] = {B00000,B01110,B11111,B10101,B11111,B01110,B00000,B00000};
byte heartEye[8] = {B00000,B01010,B10101,B01110,B01110,B00100,B00000,B00000};
byte sleepyEye[8] = {B00000,B00000,B00000,B01110,B01010,B00000,B00000,B00000};
byte winkEye[8] = {B00000,B01110,B11111,B11011,B11111,B01110,B00000,B00000};
byte winkClosed[8] = {B00000,B00100,B01110,B11111,B01110,B00100,B00000,B00000};
byte tongueOut[8] = {B00000,B00000,B00000,B01110,B01110,B00100,B00100,B00000};
byte kissMouth[8] = {B00000,B00000,B00000,B00100,B01110,B00100,B00000,B00000};
byte giggleMouth[8] = {B00000,B00000,B10101,B01110,B10101,B00000,B00000,B00000};

float readings[READINGS_COUNT], averageDistance = 0;
int readingIndex = 0, percentage = 0;

float measureDistance();
float calculateAverage();
int distanceToPercentage(float distance);
void displayTrashBin(int percentage);
void displayStatus(int percentage);
void drawCura(bool eyesClosed, int emoteType);
void drawCuraNormal(bool eyesClosed, int emoteType);
void updateCuraAnimation();
void checkIdleState(int currentPercentage);
void displayStartupMessage();
void loadSpecialEmoteChars(int emoteType);
float smoothDistance(float newDistance);

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2); lcd.clear(); lcd.backlight();
  
  byte* chars[] = {openEye, closedEye, neutralMouth, smileMouth, uwuMouth, worriedMouth, happyEye, worriedEye};
  for(int i = 0; i < 8; i++) lcd.createChar(i, chars[i]);
  
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  for (int i = 0; i < READINGS_COUNT; i++) readings[i] = EMPTY_DISTANCE;
  randomSeed(analogRead(A0));
  
  nextBlinkDelay = random(2000, 5000); lastBlinkTime = millis();
  nextEmoteDelay = random(8000, 15000); lastEmoteTime = millis();
  
  displayStartupMessage(); delay(3000); lcd.clear();
}

void loop() {
  unsigned long t = millis();
  long interval = isIdle ? IDLE_MEASUREMENT_INTERVAL : MEASUREMENT_INTERVAL;
  
  if (t - lastMeasurementTime >= interval) {
    float newDist = smoothDistance(measureDistance());
    readings[readingIndex] = newDist;
    readingIndex = (readingIndex + 1) % READINGS_COUNT;
    averageDistance = calculateAverage();
    
    int newPct = distanceToPercentage(averageDistance);
    if (abs(newPct - stablePercentage) >= 1 || abs(averageDistance - lastDisplayedDistance) >= CHANGE_THRESHOLD) {
      stablePercentage = newPct; lastDisplayedDistance = averageDistance; percentage = stablePercentage;
    }
    
    checkIdleState(percentage); lastMeasurementTime = t;
    Serial.print("Distance: "); Serial.print(averageDistance); Serial.print(" cm, Percentage: "); 
    Serial.print(percentage); Serial.print("%, Idle: "); Serial.println(isIdle ? "Yes" : "No");
  }
  
  updateCuraAnimation();
  if (!isIdle) { displayTrashBin(percentage); displayStatus(percentage); }
  delay(100);
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10); digitalWrite(TRIG_PIN, LOW);
  float distance = (pulseIn(ECHO_PIN, HIGH) * 0.034) / 2;
  return (distance > 400 || distance < 2) ? EMPTY_DISTANCE : distance;
}

float calculateAverage() {
  float sum = 0;
  for (int i = 0; i < READINGS_COUNT; i++) sum += readings[i];
  return sum / READINGS_COUNT;
}

int distanceToPercentage(float distance) {
  if (distance >= EMPTY_DISTANCE) return 0;
  if (distance <= 1) return 100;
  return constrain((int)((EMPTY_DISTANCE - distance) / EMPTY_DISTANCE * 100), 0, 100);
}

void displayTrashBin(int pct) {
  if (pct != lastDisplayedPercentage) {
    lcd.setCursor(0, 0); lcd.print("Trash: "); 
    if (pct < 10) lcd.print(" "); lcd.print(pct); lcd.print("%   ");
    
    lcd.setCursor(0, 1); lcd.print("[");
    int bars = map(pct, 0, 100, 0, 8);
    for (int i = 0; i < 8; i++) lcd.print(i < bars ? char(255) : " ");
    lcd.print("]   ");
    lastDisplayedPercentage = pct;
  }
  drawCuraNormal(isBlinking, isSpecialEmote ? currentEmote : 0);
}

void displayStatus(int percentage) {
  lcd.setCursor(0, 1);
  if (averageDistance >= 1 && averageDistance <= 3) lcd.print("Almost Full!    ");
  else if (averageDistance >= 4 && averageDistance <= 8) lcd.print("Getting Full    ");
  else lcd.print("                ");
}

void drawCura(bool eyesClosed, int emoteType) {
  if (needsRedraw || eyesClosed != lastBlinkState || emoteType != lastEmoteType) {
    lcd.clear();
    
    loadSpecialEmoteChars(emoteType);
    
    lcd.setCursor(7, 0);
    const byte leftEyes[] = {0, 6, 7, 6, 6, 0, 6};
    lcd.write(eyesClosed ? 1 : leftEyes[emoteType < 7 ? emoteType : 0]);
    
    lcd.setCursor(9, 0);
    const byte rightEyes[] = {0, 6, 7, 6, 6, 0, 1};
    lcd.write(eyesClosed ? 1 : rightEyes[emoteType < 7 ? emoteType : 0]);
    
    lcd.setCursor(8, 1);
    const byte mouths[] = {3, 4, 5, 7, 2, 7, 7};
    lcd.write(mouths[emoteType < 7 ? emoteType : 0]);
    
    lcd.setCursor(0, 1);
    lcd.print(percentage);
    lcd.print("%");
    
    lastBlinkState = eyesClosed;
    lastEmoteType = emoteType;
    needsRedraw = false;
  }
}

void drawCuraNormal(bool eyesClosed, int emoteType) {
  loadSpecialEmoteChars(emoteType);
  
  lcd.setCursor(13, 0);
  const byte leftEyes[] = {0, 6, 7, 6, 6, 0, 6};
  lcd.write(eyesClosed ? 1 : leftEyes[emoteType < 7 ? emoteType : 0]);
  
  lcd.setCursor(15, 0);
  const byte rightEyes[] = {0, 6, 7, 6, 6, 0, 1};
  lcd.write(eyesClosed ? 1 : rightEyes[emoteType < 7 ? emoteType : 0]);
  
  lcd.setCursor(14, 1);
  const byte mouths[] = {3, 4, 5, 7, 2, 7, 7};
  lcd.write(mouths[emoteType < 7 ? emoteType : 0]);
}

void updateCuraAnimation() {
  unsigned long t = millis();
  
  if (!isBlinking && t - lastBlinkTime >= nextBlinkDelay) {
    isBlinking = true; blinkStartTime = t;
  } else if (isBlinking && t - blinkStartTime >= BLINK_DURATION) {
    isBlinking = false; lastBlinkTime = t; nextBlinkDelay = random(2000, 6000);
  }
  
  if (!isSpecialEmote && t - lastEmoteTime >= nextEmoteDelay) {
    isSpecialEmote = true; emoteStartTime = t;
    if (percentage >= 85) currentEmote = 2;
    else { int emotes[] = {0,1,3,4,5,6}; currentEmote = emotes[random(0,6)]; }
  } else if (isSpecialEmote && t - emoteStartTime >= EMOTE_DURATION) {
    isSpecialEmote = false; lastEmoteTime = t; nextEmoteDelay = random(5000, 12000); currentEmote = 0;
  }
  
  if (isIdle) drawCura(isBlinking, isSpecialEmote ? currentEmote : 0);
}

void checkIdleState(int pct) {
  if (pct == lastPercentage) {
    if (++samePercentageCount >= IDLE_THRESHOLD && !isIdle) {
      isIdle = true; lcd.clear(); needsRedraw = true; Serial.println("Entering idle mode");
    }
  } else {
    samePercentageCount = 0;
    if (isIdle) { isIdle = false; lcd.clear(); needsRedraw = true; Serial.println("Exiting idle mode"); }
  }
  lastPercentage = pct;
}

void displayStartupMessage() {
  lcd.setCursor(0, 0); lcd.print("Cura the Smart");
  lcd.setCursor(0, 1); lcd.print("Trash Bin ^w^"); delay(2000);
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Starting up...");
  lcd.setCursor(0, 1); lcd.write(6); lcd.setCursor(1, 1); lcd.write(4); lcd.setCursor(2, 1); lcd.write(6);
}

void loadSpecialEmoteChars(int emoteType) {
  if (emoteType == 3) { lcd.createChar(6, heartEye); lcd.createChar(7, kissMouth); }
  else if (emoteType == 4) { lcd.createChar(6, sleepyEye); }
  else if (emoteType == 5) { lcd.createChar(7, tongueOut); }
  else if (emoteType == 6) { lcd.createChar(6, winkEye); lcd.createChar(7, giggleMouth); }
  else { lcd.createChar(6, happyEye); lcd.createChar(7, worriedEye); }
}

float smoothDistance(float newDist) {
  static float smoothed = 0; static bool first = true;
  if (first) { smoothed = newDist; first = false; } 
  else smoothed = (0.3 * newDist) + (0.7 * smoothed);
  return smoothed;
}
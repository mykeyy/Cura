#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Ultrasonic sensor pins
const int trigPin = 10;
const int echoPin = 11;

// Constants
const float EMPTY_DISTANCE = 22.0; // 22cm = 0% (empty)
const int READINGS_COUNT = 12;      // Increased for more stability
const long MEASUREMENT_INTERVAL = 200; // Slower measurements for stability
const long IDLE_MEASUREMENT_INTERVAL = 4000; // Slower measurements when idle

// Smoothing and stability
const float CHANGE_THRESHOLD = 2.0; // Minimum change needed to update display
float lastDisplayedDistance = 0;
int stablePercentage = 0;

// Idle state variables
bool isIdle = false;
int lastPercentage = -1;
int samePercentageCount = 0;
const int IDLE_THRESHOLD = 3; // Number of same readings to enter idle
unsigned long lastMeasurementTime = 0;

// Cura animation variables
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkDelay = 0;
bool isBlinking = false;
unsigned long blinkStartTime = 0;
const int blinkDuration = 150;

unsigned long lastEmoteTime = 0;
unsigned long nextEmoteDelay = 0;
bool isSpecialEmote = false;
unsigned long emoteStartTime = 0;
const int emoteDuration = 2000;
int currentEmote = 0; // 0=smile, 1=uwu, 2=worried(almost full), 3=heart eyes, 4=sleepy, 5=tongue out, 6=wink

// Animation state tracking to prevent unnecessary redraws
bool lastBlinkState = false;
int lastEmoteType = -1;
bool needsRedraw = true;
int lastDisplayedPercentage = -1; // Track displayed percentage for normal mode

// Cura's custom characters for cute expressions
byte openEye[8] = {
  B00000,
  B01110,
  B11111,
  B11011,
  B11111,
  B01110,
  B00000,
  B00000
};

byte closedEye[8] = {
  B00000,
  B00000,
  B01110,
  B11111,
  B01110,
  B00000,
  B00000,
  B00000
};

byte neutralMouth[8] = {
  B00000,
  B00000,
  B00000,
  B01110,
  B01110,
  B00000,
  B00000,
  B00000
};

byte smileMouth[8] = {
  B00000,
  B00000,
  B00000,
  B10001,
  B01110,
  B00000,
  B00000,
  B00000
};

byte uwuMouth[8] = {
  B00000,
  B00000,
  B00000,
  B01010,
  B00100,
  B00000,
  B00000,
  B00000
};

byte worriedMouth[8] = {
  B00000,
  B00000,
  B00000,
  B01110,
  B10001,
  B00000,
  B00000,
  B00000
};

byte happyEye[8] = {
  B00000,
  B00000,
  B01010,
  B01110,
  B01010,
  B00000,
  B00000,
  B00000
};

byte worriedEye[8] = {
  B00000,
  B01110,
  B11111,
  B10101,
  B11111,
  B01110,
  B00000,
  B00000
};

// Additional cute emotes
byte heartEye[8] = {
  B00000,
  B01010,
  B10101,
  B01110,
  B01110,
  B00100,
  B00000,
  B00000
};

byte sleepyEye[8] = {
  B00000,
  B00000,
  B00000,
  B01110,
  B01010,
  B00000,
  B00000,
  B00000
};

byte winkEye[8] = {
  B00000,
  B01110,
  B11111,
  B11011,
  B11111,
  B01110,
  B00000,
  B00000
};

byte winkClosed[8] = {
  B00000,
  B00100,
  B01110,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000
};

byte tongueOut[8] = {
  B00000,
  B00000,
  B00000,
  B01110,
  B01110,
  B00100,
  B00100,
  B00000
};

byte kissMouth[8] = {
  B00000,
  B00000,
  B00000,
  B00100,
  B01110,
  B00100,
  B00000,
  B00000
};

byte giggleMouth[8] = {
  B00000,
  B00000,
  B10101,
  B01110,
  B10101,
  B00000,
  B00000,
  B00000
};

// Variables for distance measurement
float readings[READINGS_COUNT];
int readingIndex = 0;
float averageDistance = 0;
int percentage = 0;

// Function declarations
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
  // Initialize serial communication
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.begin();
  lcd.clear();
  lcd.backlight();
  
  // Create Cura's custom characters (we'll dynamically load them based on emotes)
  // Base characters always loaded
  lcd.createChar(0, openEye);
  lcd.createChar(1, closedEye);
  lcd.createChar(2, neutralMouth);
  lcd.createChar(3, smileMouth);
  lcd.createChar(4, uwuMouth);
  lcd.createChar(5, worriedMouth);
  lcd.createChar(6, happyEye);
  lcd.createChar(7, worriedEye);
  
  // Initialize ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Initialize readings array
  for (int i = 0; i < READINGS_COUNT; i++) {
    readings[i] = EMPTY_DISTANCE;
  }
  
  // Initialize random seed
  randomSeed(analogRead(A0));
  
  // Initialize Cura animation timings
  nextBlinkDelay = random(2000, 5000);
  lastBlinkTime = millis();
  nextEmoteDelay = random(8000, 15000);
  lastEmoteTime = millis();
  
  // Show startup message
  displayStartupMessage();
  delay(3000);
  lcd.clear();
}

void loop() {
  unsigned long currentTime = millis();
  long measurementInterval = isIdle ? IDLE_MEASUREMENT_INTERVAL : MEASUREMENT_INTERVAL;
  
  // Check if it's time to measure
  if (currentTime - lastMeasurementTime >= measurementInterval) {
    // Take a new distance measurement
    float newDistance = measureDistance();
    
    // Apply smoothing
    newDistance = smoothDistance(newDistance);
    
    // Add to readings array
    readings[readingIndex] = newDistance;
    readingIndex = (readingIndex + 1) % READINGS_COUNT;
    
    // Calculate average
    averageDistance = calculateAverage();
    
    // Convert to percentage with stability check
    int newPercentage = distanceToPercentage(averageDistance);
    
    // Only update if change is significant enough (reduces flickering)
    if (abs(newPercentage - stablePercentage) >= 1 || 
        abs(averageDistance - lastDisplayedDistance) >= CHANGE_THRESHOLD) {
      stablePercentage = newPercentage;
      lastDisplayedDistance = averageDistance;
      percentage = stablePercentage;
    }
    
    // Always check for idle state
    checkIdleState(percentage);
    
    lastMeasurementTime = currentTime;
    
    // Debug output to serial
    Serial.print("Distance: ");
    Serial.print(averageDistance);
    Serial.print(" cm, Percentage: ");
    Serial.print(percentage);
    Serial.print("%, Idle: ");
    Serial.println(isIdle ? "Yes" : "No");
  }
  
  if (isIdle) {
    // In idle mode, show full screen Cura with percentage in corner
    updateCuraAnimation();
  } else {
    // Normal operation mode - progress bar on left, small Cura on right
    updateCuraAnimation(); // Update Cura's expressions
    displayTrashBin(percentage);
    displayStatus(percentage);
  }
  
  delay(100); // Slightly longer delay to reduce unnecessary updates
}

float measureDistance() {
  // Clear the trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Send 10 microsecond pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read the echo pin
  long duration = pulseIn(echoPin, HIGH);
  
  // Calculate distance in cm
  float distance = (duration * 0.034) / 2;
  
  // Constrain to reasonable values
  if (distance > 400 || distance < 2) {
    distance = EMPTY_DISTANCE; // Default to empty if out of range
  }
  
  return distance;
}

float calculateAverage() {
  float sum = 0;
  for (int i = 0; i < READINGS_COUNT; i++) {
    sum += readings[i];
  }
  return sum / READINGS_COUNT;
}

int distanceToPercentage(float distance) {
  // Constrain distance to valid range
  if (distance >= EMPTY_DISTANCE) {
    return 0; // Empty
  }
  if (distance <= 1) {
    return 100; // Full
  }
  
  // Calculate percentage (inverted - smaller distance = higher percentage)
  int percent = (int)((EMPTY_DISTANCE - distance) / EMPTY_DISTANCE * 100);
  
  // Constrain to 0-100%
  return constrain(percent, 0, 100);
}

void displayTrashBin(int percentage) {
  // Only redraw progress bar if percentage actually changed
  if (percentage != lastDisplayedPercentage) {
    // Clear and redraw the entire display for normal mode
    lcd.setCursor(0, 0);
    lcd.print("Trash: ");
    if (percentage < 10) {
      lcd.print(" "); // Add space for single digits
    }
    lcd.print(percentage);
    lcd.print("%   ");
    
    lcd.setCursor(0, 1);
    lcd.print("[");
    
    // 8-character progress bar
    int bars = map(percentage, 0, 100, 0, 8);
    for (int i = 0; i < 8; i++) {
      if (i < bars) {
        lcd.print(char(255)); // Full block
      } else {
        lcd.print(" ");
      }
    }
    lcd.print("]   "); // Clear any remaining characters
    
    lastDisplayedPercentage = percentage;
  }
  
  // Always draw Cura for animations
  drawCuraNormal(isBlinking, isSpecialEmote ? currentEmote : 0);
}

void displayStatus(int percentage) {
  // This function is now only used in non-idle mode
  lcd.setCursor(0, 1);
  
  if (averageDistance >= 1 && averageDistance <= 3) {
    lcd.print("Almost Full!    ");
  } else if (averageDistance >= 4 && averageDistance <= 8) {
    lcd.print("Getting Full    ");
  } else {
    lcd.print("                "); // Clear status line in normal mode
  }
}

void drawCura(bool eyesClosed, int emoteType) {
  // Only clear and redraw if something actually changed
  if (needsRedraw || eyesClosed != lastBlinkState || emoteType != lastEmoteType) {
    // Full screen Cura for idle mode - clear everything first
    lcd.clear();
    
    // Load special characters for this emote
    loadSpecialEmoteChars(emoteType);
    
    // Draw eyes (centered on screen)
    lcd.setCursor(7, 0);  // Left eye
    if (eyesClosed) {
      lcd.write(1); // Closed eye
    } else if (emoteType == 1) { // UwU
      lcd.write(6); // Happy eye
    } else if (emoteType == 2) { // Worried
      lcd.write(7); // Worried eye
    } else if (emoteType == 3) { // Heart eyes
      lcd.write(6); // Use heartEye (loaded in slot 6)
    } else if (emoteType == 4) { // Sleepy
      lcd.write(6); // Use sleepyEye (loaded in slot 6)
    } else if (emoteType == 5) { // Tongue out
      lcd.write(0); // Normal eye
    } else if (emoteType == 6) { // Wink
      lcd.write(6); // Use winkEye (loaded in slot 6)
    } else {
      lcd.write(0); // Normal open eye
    }
    
    lcd.setCursor(9, 0); // Right eye
    if (eyesClosed) {
      lcd.write(1); // Closed eye
    } else if (emoteType == 1) { // UwU
      lcd.write(6); // Happy eye
    } else if (emoteType == 2) { // Worried
      lcd.write(7); // Worried eye
    } else if (emoteType == 3) { // Heart eyes
      lcd.write(6); // Use heartEye (loaded in slot 6)
    } else if (emoteType == 4) { // Sleepy
      lcd.write(6); // Use sleepyEye (loaded in slot 6)
    } else if (emoteType == 5) { // Tongue out
      lcd.write(0); // Normal eye
    } else if (emoteType == 6) { // Wink
      lcd.write(1); // Closed eye for wink
    } else {
      lcd.write(0); // Normal open eye
    }
    
    // Draw mouth (centered)
    lcd.setCursor(8, 1);
    if (emoteType == 1) { // UwU
      lcd.write(4); // UwU mouth
    } else if (emoteType == 2) { // Worried
      lcd.write(5); // Worried mouth
    } else if (emoteType == 3) { // Heart eyes
      lcd.write(7); // Use kissMouth (loaded in slot 7)
    } else if (emoteType == 4) { // Sleepy
      lcd.write(2); // Neutral mouth
    } else if (emoteType == 5) { // Tongue out
      lcd.write(7); // Use tongueOut (loaded in slot 7)
    } else if (emoteType == 6) { // Wink
      lcd.write(7); // Use giggleMouth (loaded in slot 7)
    } else if (emoteType == 0) { // Smile
      lcd.write(3); // Smile mouth
    } else {
      lcd.write(2); // Neutral mouth
    }
    
    // Add percentage in bottom left
    lcd.setCursor(0, 1);
    lcd.print(percentage);
    lcd.print("%");
    
    // Update state tracking
    lastBlinkState = eyesClosed;
    lastEmoteType = emoteType;
    needsRedraw = false;
  }
}

void drawCuraNormal(bool eyesClosed, int emoteType) {
  // Small Cura for normal mode on the right side
  
  // Load special characters for this emote
  loadSpecialEmoteChars(emoteType);
  
  // Draw eyes
  lcd.setCursor(13, 0);
  if (eyesClosed) {
    lcd.write(1); // Closed eye
  } else if (emoteType == 1) { // UwU
    lcd.write(6); // Happy eye
  } else if (emoteType == 2) { // Worried
    lcd.write(7); // Worried eye
  } else if (emoteType == 3) { // Heart eyes
    lcd.write(6); // Use heartEye
  } else if (emoteType == 4) { // Sleepy
    lcd.write(6); // Use sleepyEye
  } else if (emoteType == 5) { // Tongue out
    lcd.write(0); // Normal eye
  } else if (emoteType == 6) { // Wink
    lcd.write(6); // Use winkEye
  } else {
    lcd.write(0); // Normal open eye
  }
  
  lcd.setCursor(15, 0);
  if (eyesClosed) {
    lcd.write(1); // Closed eye
  } else if (emoteType == 1) { // UwU
    lcd.write(6); // Happy eye
  } else if (emoteType == 2) { // Worried
    lcd.write(7); // Worried eye
  } else if (emoteType == 3) { // Heart eyes
    lcd.write(6); // Use heartEye
  } else if (emoteType == 4) { // Sleepy
    lcd.write(6); // Use sleepyEye
  } else if (emoteType == 5) { // Tongue out
    lcd.write(0); // Normal eye
  } else if (emoteType == 6) { // Wink
    lcd.write(1); // Closed eye for wink
  } else {
    lcd.write(0); // Normal open eye
  }
  
  // Draw mouth
  lcd.setCursor(14, 1);
  if (emoteType == 1) { // UwU
    lcd.write(4); // UwU mouth
  } else if (emoteType == 2) { // Worried
    lcd.write(5); // Worried mouth
  } else if (emoteType == 3) { // Heart eyes
    lcd.write(7); // Use kissMouth
  } else if (emoteType == 4) { // Sleepy
    lcd.write(2); // Neutral mouth
  } else if (emoteType == 5) { // Tongue out
    lcd.write(7); // Use tongueOut
  } else if (emoteType == 6) { // Wink
    lcd.write(7); // Use giggleMouth
  } else if (emoteType == 0) { // Smile
    lcd.write(3); // Smile mouth
  } else {
    lcd.write(2); // Neutral mouth
  }
}

void updateCuraAnimation() {
  unsigned long currentTime = millis();
  
  // Handle blinking
  if (!isBlinking) {
    if (currentTime - lastBlinkTime >= nextBlinkDelay) {
      isBlinking = true;
      blinkStartTime = currentTime;
    }
  } else {
    if (currentTime - blinkStartTime >= blinkDuration) {
      isBlinking = false;
      lastBlinkTime = currentTime;
      nextBlinkDelay = random(2000, 6000);
    }
  }
  
  // Handle special emotes
  if (!isSpecialEmote) {
    if (currentTime - lastEmoteTime >= nextEmoteDelay) {
      isSpecialEmote = true;
      emoteStartTime = currentTime;
      
      // Choose emote based on trash level
      if (percentage >= 85) {
        currentEmote = 2; // Worried for almost full
      } else {
        // Random cute emotes for normal operation
        int randomEmotes[] = {0, 1, 3, 4, 5, 6}; // smile, uwu, heart eyes, sleepy, tongue out, wink
        currentEmote = randomEmotes[random(0, 6)];
      }
    }
  } else {
    if (currentTime - emoteStartTime >= emoteDuration) {
      isSpecialEmote = false;
      lastEmoteTime = currentTime;
      nextEmoteDelay = random(5000, 12000);
      currentEmote = 0; // Back to default smile
    }
  }
  
  // Draw Cura based on current mode
  if (isIdle) {
    drawCura(isBlinking, isSpecialEmote ? currentEmote : 0); // Full screen
  }
  // Note: Normal mode Cura is drawn in displayTrashBin()
}

void checkIdleState(int currentPercentage) {
  if (currentPercentage == lastPercentage) {
    samePercentageCount++;
    if (samePercentageCount >= IDLE_THRESHOLD && !isIdle) {
      isIdle = true;
      lcd.clear();
      needsRedraw = true; // Force redraw when entering idle
      Serial.println("Entering idle mode");
    }
  } else {
    samePercentageCount = 0;
    if (isIdle) {
      isIdle = false;
      lcd.clear();
      needsRedraw = true; // Force redraw when exiting idle
      Serial.println("Exiting idle mode");
    }
  }
  lastPercentage = currentPercentage;
}

void displayStartupMessage() {
  lcd.setCursor(0, 0);
  lcd.print("Cura the Smart");
  lcd.setCursor(0, 1);
  lcd.print("Trash Bin ^w^");
  delay(2000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting up...");
  
  // Show UwU face during startup
  lcd.setCursor(0, 1);
  lcd.write(6); // Happy eye
  lcd.setCursor(1, 1);
  lcd.write(4); // UwU mouth
  lcd.setCursor(2, 1);
  lcd.write(6); // Happy eye
}

void loadSpecialEmoteChars(int emoteType) {
  // Dynamically load special characters based on emote type
  // We'll use slots 6 and 7 for temporary special characters
  
  switch(emoteType) {
    case 3: // Heart eyes
      lcd.createChar(6, heartEye);
      lcd.createChar(7, kissMouth);
      break;
    case 4: // Sleepy
      lcd.createChar(6, sleepyEye);
      // Keep neutral mouth (already loaded)
      break;
    case 5: // Tongue out
      // Keep normal eyes (already loaded)
      lcd.createChar(7, tongueOut);
      break;
    case 6: // Wink
      lcd.createChar(6, winkEye);
      lcd.createChar(7, giggleMouth);
      break;
    default:
      // For basic emotes (0,1,2), keep original character set
      lcd.createChar(6, happyEye);
      lcd.createChar(7, worriedEye);
      break;
  }
}

float smoothDistance(float newDistance) {
  // Simple exponential smoothing for more stable readings
  static float smoothedValue = 0;
  static bool firstReading = true;
  
  if (firstReading) {
    smoothedValue = newDistance;
    firstReading = false;
  } else {
    // Apply smoothing factor (0.3 = 30% new value, 70% old value)
    smoothedValue = (0.3 * newDistance) + (0.7 * smoothedValue);
  }
  
  return smoothedValue;
}
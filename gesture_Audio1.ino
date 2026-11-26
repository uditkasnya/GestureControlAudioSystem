
#include <Wire.h>
#include <Adafruit_APDS9960.h>
#include <HijelHID_BLEKeyboard.h>

#define SDA_PIN 21
#define SCL_PIN 22
#define BUZZER_PIN 25

#define NEAR_THRESHOLD 10
#define RELEASE_THRESHOLD 5

// Change this value to adjust Play/Pause hold time.
// 1000 = 1 second, 2000 = 2 seconds, 500 = 0.5 seconds.
#define PLAY_PAUSE_HOLD_TIME 1000

Adafruit_APDS9960 apds;
HijelHID_BLEKeyboard keyboard;/*
  CORRIDOR LIGHT AUTOMATION SYSTEM
  Arduino UNO + PIR + LDR + 1 LED
*/

const int pirPin = 2;
const int ldrPin = A0;
const int ledPin = 5;

// Adjust after checking LDR readings
const int darkThreshold = 500;

void setup() {

  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(ledPin, LOW);

  Serial.begin(9600);

  Serial.println("Corridor Light Automation");
  Serial.println("PIR Sensor Initializing...");

  delay(30000);

  Serial.println("System Ready!");
}

void loop() {

  int ldrValue = analogRead(ldrPin);
  int pirValue = digitalRead(pirPin);

  Serial.print("LDR Value: ");
  Serial.print(ldrValue);

  Serial.print(" | PIR Value: ");
  Serial.println(pirValue);

  // Dark + Motion = LED ON
  if (ldrValue < darkThreshold && pirValue == HIGH) {

    digitalWrite(ledPin, HIGH);
    Serial.println("LED ON - Motion Detected");

  }
  else {

    digitalWrite(ledPin, LOW);
    Serial.println("LED OFF");

  }

  delay(300);
}

const int VOLUME_STEPS = 5;

const unsigned long PROXIMITY_WINDOW = 100;
const unsigned long GESTURE_WINDOW = 400;
const unsigned long GESTURE_COOLDOWN = 700;

unsigned long nearStartTime = 0;
unsigned long lastGestureTime = 0;
unsigned long modeStartTime = 0;

bool nearTiming = false;
bool playPauseTriggered = false;
bool proximityMode = true;

// ---------------- BUZZER FUNCTIONS ----------------

void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void nearBeep() {
  beep(100);
}

void farBeep() {
  beep(80);
  delay(100);
  beep(80);
}

void upBeep() {
  beep(60);
}

void downBeep() {
  beep(200);
}

void leftBeep() {
  beep(70);
  delay(100);
  beep(70);
}

void rightBeep() {
  beep(70);
  delay(80);
  beep(70);
  delay(80);
  beep(70);
}

void playPauseBeep() {
  beep(250);
}

// ---------------- SETUP ----------------

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("GESTURE AUDIO CONTROLLER");

  if (!apds.begin()) {
    Serial.println("APDS9960 NOT DETECTED!");
    while (1) delay(100);
  }

  apds.enableGesture(false);
  apds.enableProximity(true);

  keyboard.begin();

  Serial.println("Bluetooth started!");
  Serial.println("Pair with Windows laptop.");
}

// ---------------- MEDIA FUNCTIONS ----------------

void volumeUp() {
  Serial.println("VOLUME +10% APPROX");

  for (int i = 0; i < VOLUME_STEPS; i++) {
    keyboard.tap(MEDIA_VOLUME_UP);
    delay(30);
  }

  upBeep();
}

void volumeDown() {
  Serial.println("VOLUME -10% APPROX");

  for (int i = 0; i < VOLUME_STEPS; i++) {
    keyboard.tap(MEDIA_VOLUME_DOWN);
    delay(30);
  }

  downBeep();
}

void playPause() {
  Serial.println("NEAR HOLD -> PLAY / PAUSE");

  keyboard.tap(MEDIA_PLAY_PAUSE);
  playPauseBeep();
}

void nextTrack() {
  Serial.println("RIGHT -> NEXT TRACK");

  keyboard.tap(MEDIA_NEXT_TRACK);
  rightBeep();
}

void previousTrack() {
  Serial.println("LEFT -> PREVIOUS TRACK");

  keyboard.tap(MEDIA_PREV_TRACK);
  leftBeep();
}

// ---------------- MAIN LOOP ----------------

void loop() {

  if (!keyboard.isConnected()) {
    delay(100);
    return;
  }

  unsigned long now = millis();

  // ================ PROXIMITY MODE ================

  if (proximityMode) {

    uint8_t proximity = apds.readProximity();

    // NEAR DETECTION
    if (proximity >= NEAR_THRESHOLD) {

      if (!nearTiming) {
        nearStartTime = now;
        nearTiming = true;
        playPauseTriggered = false;

        Serial.println("NEAR DETECTED");
        nearBeep();
      }

      // PLAY / PAUSE AFTER SELECTED HOLD TIME
      if (!playPauseTriggered &&
          now - nearStartTime >= PLAY_PAUSE_HOLD_TIME) {

        playPause();
        playPauseTriggered = true;
        lastGestureTime = millis();
      }

    }

    // FAR / HAND RELEASE DETECTION
    else if (proximity <= RELEASE_THRESHOLD) {

      if (nearTiming) {
        Serial.println("FAR DETECTED");
        farBeep();
      }

      nearTiming = false;
      playPauseTriggered = false;
    }

    if (millis() - modeStartTime >= PROXIMITY_WINDOW) {

      apds.enableProximity(false);
      apds.enableGesture(true);

      proximityMode = false;
      modeStartTime = millis();
    }
  }

  // ================ GESTURE MODE ================

  else {

    uint8_t gesture = apds.readGesture();

    if (gesture != 0 &&
        millis() - lastGestureTime >= GESTURE_COOLDOWN) {

      lastGestureTime = millis();

      switch (gesture) {

        case APDS9960_UP:
          volumeUp();
          break;

        case APDS9960_DOWN:
          volumeDown();
          break;

        case APDS9960_RIGHT:
          nextTrack();
          break;

        case APDS9960_LEFT:
          previousTrack();
          break;
      }
    }

    if (millis() - modeStartTime >= GESTURE_WINDOW) {

      apds.enableGesture(false);
      apds.enableProximity(true);

      proximityMode = true;
      modeStartTime = millis();
    }
  }
}

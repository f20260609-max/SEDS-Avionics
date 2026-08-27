/*
 * SEDS BPHC - Avionics Induction Round 1
 * Task 2: Keeping Watch Over Odysseus
 * Author: Saptarshi Nandi
 * ID: [2026A4PS0609H]
 */

#include <LiquidCrystal.h>

// ---------------------------------------------------------
// Pin Mapping
// ---------------------------------------------------------
// LCD: RS=12, E=11, DB4=5, DB5=4, DB6=3, DB7=2
const int PIN_RS     = 12;
const int PIN_EN     = 11;
const int PIN_D4     = 5;
const int PIN_D5     = 4;
const int PIN_D6     = 3;
const int PIN_D7_LCD = 2;

// Peripherals
const int PIN_PING   = 9;   // 3-pin ultrasonic sensor SIG pin
const int PIN_LDR    = A0;  // Ambient light sensor
const int PIN_BUTTON = 7;   // Push button (Anchor toggle)
const int PIN_LED    = 13;  // Storm alert LED
const int PIN_BUZZER = 10;  // Charybdis buzzer

LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7_LCD);

// ---------------------------------------------------------
// State Constants
// ---------------------------------------------------------
const int STATE_OPEN_SEA        = 0;
const int STATE_ANCHOR_DROPPED  = 1;
const int STATE_STORM           = 2;
const int STATE_CHARYBDIS       = 3;
const int STATE_WRECKED         = 4;

int currentState = STATE_OPEN_SEA;
int lastRenderedState = -1;

// Thresholds 
const int LIGHT_THRESHOLD = 505;            // if the light sensor reading is below half(in the serial monitor) = Storm will be activated
const float DIST_THRESHOLD = 100.0;         // if the distance sensed by the ultarsonic sensor is below 100cm = Charybdis will be activated
const unsigned long DANGER_LIMIT_MS = 5000; // If the ship remains more than 5s  in  continuous danger = it will be wrecked

// Timers & State Variables
unsigned long dangerStartTime = 0;
unsigned long lastLedBlinkTime = 0;
bool ledState = false;

// Button Debounce & State Tracking
int lastDebouncedButtonState = HIGH;
int lastRawButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// ---------------------------------------------------------
// Ultrasonic Sensor Routine (3-Pin Ping)))
// ---------------------------------------------------------
float readDistanceCM() {
  pinMode(PIN_PING, OUTPUT);
  digitalWrite(PIN_PING, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_PING, HIGH);
  delayMicroseconds(5);
  digitalWrite(PIN_PING, LOW);

  pinMode(PIN_PING, INPUT);
  long duration = pulseIn(PIN_PING, HIGH, 30000);
  if (duration == 0) return 400.0;
  return (duration * 0.0343) / 2.0;
}

// ---------------------------------------------------------
// Reliable LCD Renderer
// ---------------------------------------------------------
void printScreen(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void renderStateLCD(int state) {
  switch (state) {
    case STATE_OPEN_SEA:
      printScreen("State: OPEN SEA", "Sailing Calmly");
      break;
    case STATE_ANCHOR_DROPPED:
      printScreen("ANCHOR DROPPED", "Safe & Protected");
      break;
    case STATE_STORM:
      printScreen("State: STORM", "Blinking LED");
      break;
    case STATE_CHARYBDIS:
      printScreen("State: CHARYBDIS", "Sounding Buzzer");
      break;
    case STATE_WRECKED:
      printScreen("State: WRECKED", "Ship Destroyed!");
      break;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_LED, LOW);
  noTone(PIN_BUZZER);

  // Sync initial button states to avoid startup false triggers
  int initialRead = digitalRead(PIN_BUTTON);
  lastRawButtonReading = initialRead;
  lastDebouncedButtonState = initialRead;

  delay(150);
  lcd.begin(16, 2);
  lcd.clear();
  delay(50);

  renderStateLCD(STATE_OPEN_SEA);
  lastRenderedState = STATE_OPEN_SEA;
}

void loop() {
  // If wrecked, the ship holds state permanently until simulation is restarted
  if (currentState == STATE_WRECKED) {
    digitalWrite(PIN_LED, LOW);
    noTone(PIN_BUZZER);
    if (lastRenderedState != STATE_WRECKED) {
      renderStateLCD(STATE_WRECKED);
      lastRenderedState = STATE_WRECKED;
    }
    return;
  }

  // ---------------------------------------------------------
  // 1. Push Button Handling (Debounced Falling-Edge Trigger)
  // ---------------------------------------------------------
  int currentRaw = digitalRead(PIN_BUTTON);

  if (currentRaw != lastRawButtonReading) {
    lastDebounceTime = millis();
    lastRawButtonReading = currentRaw;
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Check if the debounced state actually changed
    if (currentRaw != lastDebouncedButtonState) {
      lastDebouncedButtonState = currentRaw;

      // Only trigger on active button press (HIGH -> LOW)
      if (lastDebouncedButtonState == LOW) {
        if (currentState == STATE_ANCHOR_DROPPED) {
          currentState = STATE_OPEN_SEA;
        } else {
          currentState = STATE_ANCHOR_DROPPED;
          dangerStartTime = 0;           // Reset 5s danger timer
          digitalWrite(PIN_LED, LOW);     // Silence storm LED
          noTone(PIN_BUZZER);            // Silence Charybdis buzzer
        }
      }
    }
  }

  // ---------------------------------------------------------
  // 2. Anchor State Protection
  // ---------------------------------------------------------
  if (currentState == STATE_ANCHOR_DROPPED) {
    digitalWrite(PIN_LED, LOW);
    noTone(PIN_BUZZER);
    if (lastRenderedState != STATE_ANCHOR_DROPPED) {
      renderStateLCD(STATE_ANCHOR_DROPPED);
      lastRenderedState = STATE_ANCHOR_DROPPED;
    }
    return;                                                       // the ship is completely immune to dangers while anchored
  }

  // ---------------------------------------------------------
  // 3. Sensor Reading & Evaluation
  // ---------------------------------------------------------
  int lightVal = analogRead(PIN_LDR);
  float distanceVal = readDistanceCM();
  
  Serial.println(lightVal);

  bool stormActive = ( lightVal < LIGHT_THRESHOLD);
  bool charybdisActive = (distanceVal < DIST_THRESHOLD);
   
  // ---------------------------------------------------------
  // 4. Finite State Machine Transitions
  // ---------------------------------------------------------
  switch (currentState) {
    case STATE_OPEN_SEA:
      digitalWrite(PIN_LED, LOW);
      noTone(PIN_BUZZER);
      dangerStartTime = 0;

      if (stormActive) {
        currentState = STATE_STORM;
        dangerStartTime = millis();
        lastLedBlinkTime = millis();
        ledState = true;
        digitalWrite(PIN_LED, HIGH);
      } else if (charybdisActive) {
        currentState = STATE_CHARYBDIS;
        dangerStartTime = millis();
        tone(PIN_BUZZER, 1000);
      }
      break;

    case STATE_STORM:
      // Non-blocking 250ms LED blink
      if (millis() - lastLedBlinkTime >= 250) {
        lastLedBlinkTime = millis();
        ledState = !ledState;
        digitalWrite(PIN_LED, ledState ? HIGH : LOW);
      }
      noTone(PIN_BUZZER);

      // Precedence check: if storm is exited before 5s
      if (!stormActive) {
        digitalWrite(PIN_LED, LOW);                          // This part of the code is similar to the next part... I have already commented there.
        if (charybdisActive) {
          currentState = STATE_CHARYBDIS;
          dangerStartTime = millis();
          tone(PIN_BUZZER, 1000);
        } else {
          currentState = STATE_OPEN_SEA;
        }
      } else if (millis() - dangerStartTime >= DANGER_LIMIT_MS) {
        currentState = STATE_WRECKED;
        digitalWrite(PIN_LED, LOW);
      }
      break;

    case STATE_CHARYBDIS:
      tone(PIN_BUZZER, 1000);
      digitalWrite(PIN_LED, LOW);

      // Precedence check: if Charybdis is exited before 5s
      if (!charybdisActive) {
        noTone(PIN_BUZZER);                                        // This part of the code decides what happens to the ship while exiting charybdis... like for example if the ship still remains in storm the timer will start agin for storm ... and if the ship is safe... it will show open sea and safe sailing.
        if (stormActive) {
          currentState = STATE_STORM;                       
          dangerStartTime = millis();
          lastLedBlinkTime = millis();
          ledState = true;
          digitalWrite(PIN_LED, HIGH);
        } else {
          currentState = STATE_OPEN_SEA;
        }
      } else if (millis() - dangerStartTime >= DANGER_LIMIT_MS) {
        currentState = STATE_WRECKED;
        noTone(PIN_BUZZER);
      }
      break;

    default:
      break;
  }

  // Update screen only upon state transition
  if (currentState != lastRenderedState) {   // this part of the code changes the LCD screen writing 
    renderStateLCD(currentState);
    lastRenderedState = currentState;
  }
}
   // Thank's a lot for reading.... this code couldn't have been done if not for the help of Gemini and Claude...
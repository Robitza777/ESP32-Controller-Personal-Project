#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MAX1704X.h>
#include <BleGamepad.h>

// ──────────────────────────────────────────────
// I²C Battery Fuel Gauge (MAX17048)
// ──────────────────────────────────────────────
Adafruit_MAX17048 maxlipo;

// ──────────────────────────────────────────────
// Pin Definitions
// ──────────────────────────────────────────────

// Buttons
#define A_BUTTON        23
#define B_BUTTON        3    // RX0, ok for button
#define X_BUTTON        0    // Boot-sensitive
#define Y_BUTTON        19

// Shoulders & Triggers
#define R1_BUTTON       16
#define R2_BUTTON       15   // Boot strapping, avoid held at reset
#define L1_BUTTON       18
#define L2_BUTTON       17

// Options / Start
#define OPTIONS_BUTTON  27   // Moved from 12

// Joystick Clicks
#define L3_BUTTON       26
#define R3_BUTTON       25

// D-pad
#define DPAD_UP         5
#define DPAD_DOWN       4
#define DPAD_LEFT       2
#define DPAD_RIGHT      13

// Back Paddles
#define L4_BUTTON       14
#define R4_BUTTON       12   // Boot-sensitive, fine if not held at reset

// Joysticks (Analog)
#define LEFT_VRX_JOYSTICK   34
#define LEFT_VRY_JOYSTICK   35
#define RIGHT_VRX_JOYSTICK  32
#define RIGHT_VRY_JOYSTICK  33

// ──────────────────────────────────────────────
// Configuration Constants
// ──────────────────────────────────────────────
#define NUM_BUTTONS 17
const int numberOfSamples = 5;
float alpha = 0.1; // smoothing factor

// ──────────────────────────────────────────────
// Button Mapping
// ──────────────────────────────────────────────
int buttonPins[NUM_BUTTONS] = {
  A_BUTTON, B_BUTTON, X_BUTTON, Y_BUTTON,
  R1_BUTTON, R2_BUTTON, L1_BUTTON, L2_BUTTON,
  OPTIONS_BUTTON, L3_BUTTON, R3_BUTTON,
  DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT,
  L4_BUTTON, R4_BUTTON
};

int gamepadButtonIDs[NUM_BUTTONS] = {
  1, 2, 3, 4,
  6, 8, 5, 7,
  10, 11, 12,
  13, 14, 15, 16,
  17, 18
};

// ──────────────────────────────────────────────
// BLE Gamepad setup
// ──────────────────────────────────────────────
BleGamepad bleGamepad("ESP32 Controller", "Robitza Electrical Engineer");
BleGamepadConfiguration bleConfig;

// Joystick filtered values
float filteredLeftVrx = 0;
float filteredLeftVry = 0;
float filteredRightVrx = 0;
float filteredRightVry = 0;

// ──────────────────────────────────────────────
// Battery Reading (MAX17048)
// ──────────────────────────────────────────────
int readBatteryLevel() {
  float cellVoltage = maxlipo.cellVoltage();
  float percentage = maxlipo.cellPercent();

  // safety clamp
  percentage = constrain(percentage, 0, 100);

  Serial.print("Battery: ");
  Serial.print(cellVoltage);
  Serial.print(" V  (");
  Serial.print(percentage);
  Serial.println("%)");

  return (int)percentage;
}

// ──────────────────────────────────────────────
// Setup
// ──────────────────────────────────────────────
void setup() {
  delay(1000);
  Serial.begin(115200);

  // Initialize I2C for MAX17048
  Wire.begin(21, 22);
  if (!maxlipo.begin(&Wire)) {
    Serial.println("MAX17048 not found. Check wiring!");
  } else {
    Serial.println("MAX17048 detected.");
  }

  // Set up buttons
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // BLE Gamepad configuration
  bleConfig.setAutoReport(false);
  bleConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD);
  bleConfig.setVid(0x054C); // Sony VID
  bleConfig.setPid(0x0CE6); // DualSense PID
  bleGamepad.begin(&bleConfig);
}

// ──────────────────────────────────────────────
// Loop
// ──────────────────────────────────────────────
void loop() {
  if (bleGamepad.isConnected()) {
    // Read and average joystick values
    int totalLX = 0, totalLY = 0, totalRX = 0, totalRY = 0;

    for (int i = 0; i < numberOfSamples; i++) {
      totalLX += analogRead(LEFT_VRX_JOYSTICK);
      totalLY += analogRead(LEFT_VRY_JOYSTICK);
      totalRX += analogRead(RIGHT_VRX_JOYSTICK);
      totalRY += analogRead(RIGHT_VRY_JOYSTICK);
      delay(3);
    }

    // Average readings
    int avgLX = totalLX / numberOfSamples;
    int avgLY = totalLY / numberOfSamples;
    int avgRX = totalRX / numberOfSamples;
    int avgRY = totalRY / numberOfSamples;

    // Low-pass filtering
    filteredLeftVrx  = alpha * avgLX + (1 - alpha) * filteredLeftVrx;
    filteredLeftVry  = alpha * avgLY + (1 - alpha) * filteredLeftVry;
    filteredRightVrx = alpha * avgRX + (1 - alpha) * filteredRightVrx;
    filteredRightVry = alpha * avgRY + (1 - alpha) * filteredRightVry;

    // Map analog to joystick range
    int16_t lx = map(filteredLeftVrx, 0, 4095, -32767, 32767);
    int16_t ly = map(filteredLeftVry, 0, 4095, -32767, 32767);
    int16_t rx = map(filteredRightVrx, 0, 4095, -32767, 32767);
    int16_t ry = map(filteredRightVry, 0, 4095, -32767, 32767);

    // Buttons
    for (int i = 0; i < NUM_BUTTONS; i++) {
      if (!digitalRead(buttonPins[i])) bleGamepad.press(gamepadButtonIDs[i]);
      else bleGamepad.release(gamepadButtonIDs[i]);
    }

    // Joystick updates
    bleGamepad.setX(lx);
    bleGamepad.setY(ly);
    bleGamepad.setZ(rx);
    bleGamepad.setRX(ry);

    // Battery percentage from MAX17048
    bleGamepad.setBatteryLevel(readBatteryLevel());

    // Send to host
    bleGamepad.sendReport();
  }

  delay(20);
}

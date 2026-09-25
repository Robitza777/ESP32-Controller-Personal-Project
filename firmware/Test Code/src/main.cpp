#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
#include <BleGamepad.h>

// Define Standard I2C Pins
#define I2C_SDA 21
#define I2C_SCL 22

// OLED Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// MAX17048 Setup
SFE_MAX1704X lipo;

// BLE Gamepad Setup
BleGamepad bleGamepad("Custom ESP32 Gamepad", "DIY", 100);

// Analog Pins
constexpr uint8_t PIN_LX = 34;
constexpr uint8_t PIN_LY = 35;
constexpr uint8_t PIN_RX = 32;
constexpr uint8_t PIN_RY = 33;

constexpr int ADC_CENTER = 2048;
constexpr int DEADZONE   = 100;

// Buttons Struct
struct ButtonPin {
  const char* name;
  uint8_t pin;
  bool externalPull; // true for GPIO 36, 39 and 2
  uint16_t bleMask;
};

// Buttons Mapping
ButtonPin buttons[] = {
  {"PRF", 36, true,  BUTTON_1},
  {"L3",  25, false, BUTTON_9},
  {"R3",  26, false, BUTTON_10},
  {"OPT", 27, false, BUTTON_8},
  {"L4",  14, false, BUTTON_11},
  {"R4",  12, false, BUTTON_12},
  {"D-R", 13, false, 0},         // D-pad Right
  {"D-L", 23, false, 0},         // D-pad Left
  {"D-U", 5,  false, 0},         // D-pad Up
  {"D-D", 39, true,  0},         // D-pad Down
  {"A",   0,  false, BUTTON_1},  // Cross / A
  {"B",   17, false, BUTTON_2},  // Circle / B
  {"X",   2,  true,  BUTTON_3},  // Square / X
  {"Y",   4,  false, BUTTON_4},  // Triangle / Y
  {"L1",  18, false, BUTTON_5},
  {"L2",  19, false, BUTTON_7},
  {"R1",  16, false, BUTTON_6},
  {"R2",  15, false, BUTTON_8}
};

constexpr size_t NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

bool oledFound = false;
bool lipoFound = false;
uint32_t lastLoopUpdate = 0;

int16_t processAxis(int rawValue, bool invert = false) {
  int diff = rawValue - ADC_CENTER;
  if (abs(diff) < DEADZONE) return 0;
  long mapped = map(rawValue, 0, 4095, -32767, 32767);
  mapped = constrain(mapped, -32767, 32767);
  return invert ? -mapped : mapped;
}

void setup() {
  Serial.begin(115200);

  // I2C Initialization
  Wire.begin(I2C_SDA, I2C_SCL);

  // Quick I2C Scan
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      if (address == 0x3C) oledFound = true;
      if (address == 0x36) lipoFound = true;
    }
  }

  // Display Initialization and Rotation
  if (oledFound) {
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      display.setRotation(2); // 180 Degrees Spin
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("Controller Initializing");
      display.println("BLE Advertising...");
      display.display();
    }
  }

  // Fuel Gauge Initialization
  if (lipoFound) {
    lipo.begin();
  }

  // Button Pins Initialization
  for (size_t i = 0; i < NUM_BUTTONS; i++) {
    if (buttons[i].externalPull) {
      pinMode(buttons[i].pin, INPUT);
    } else {
      pinMode(buttons[i].pin, INPUT_PULLUP);
    }
  }

  analogReadResolution(12);

  // Bluetooth Transmition Initialization (Currently only to have a consumption high enough to keep the ip5306's 5v output on)
  BleGamepadConfiguration bleGamepadConfig;
  bleGamepadConfig.setAutoReport(false);
  bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD);
  bleGamepad.begin(&bleGamepadConfig);

  Serial.println("\n[SYSTEM] BLE + Hardware on. Radio active.");
}

void loop() {
  uint32_t now = millis();

  // 50 Hz refresh rate
  if (now - lastLoopUpdate >= 20) {
    lastLoopUpdate = now;

    // 1. Buttons Read
    int pressedCount = 0;
    String pressedNames = "";

    bool dUp    = (digitalRead(5)  == LOW);
    bool dDown  = (digitalRead(39) == LOW);
    bool dLeft  = (digitalRead(23) == LOW);
    bool dRight = (digitalRead(13) == LOW);

    for (size_t i = 0; i < NUM_BUTTONS; i++) {
      bool isPressed = (digitalRead(buttons[i].pin) == LOW);
      if (isPressed) {
        pressedCount++;
        pressedNames += buttons[i].name;
        pressedNames += " ";
      }

      if (bleGamepad.isConnected() && buttons[i].bleMask != 0) {
        if (isPressed) bleGamepad.press(buttons[i].bleMask);
        else bleGamepad.release(buttons[i].bleMask);
      }
    }

    // 2. D-pad Mapping
    if (bleGamepad.isConnected()) {
      if      (dUp && dRight)    bleGamepad.setHat(DPAD_UP_RIGHT);
      else if (dUp && dLeft)     bleGamepad.setHat(DPAD_UP_LEFT);
      else if (dDown && dRight)  bleGamepad.setHat(DPAD_DOWN_RIGHT);
      else if (dDown && dLeft)   bleGamepad.setHat(DPAD_DOWN_LEFT);
      else if (dUp)              bleGamepad.setHat(DPAD_UP);
      else if (dDown)            bleGamepad.setHat(DPAD_DOWN);
      else if (dLeft)            bleGamepad.setHat(DPAD_LEFT);
      else if (dRight)           bleGamepad.setHat(DPAD_RIGHT);
      else                       bleGamepad.setHat(DPAD_CENTERED);
    }

    // 3. Analog Axis Read And Mapping
    int rawLX = analogRead(PIN_LX);
    int rawLY = analogRead(PIN_LY);
    int rawRX = analogRead(PIN_RX);
    int rawRY = analogRead(PIN_RY);

    int16_t lx = processAxis(rawLX);
    int16_t ly = processAxis(rawLY, true);
    int16_t rx = processAxis(rawRX);
    int16_t ry = processAxis(rawRY, true);

    if (bleGamepad.isConnected()) {
      bleGamepad.setLeftThumb(lx, ly);
      bleGamepad.setRightThumb(rx, ry);
      bleGamepad.sendReport();
    }

    // 4. Battery Reading
    float vCell = lipoFound ? lipo.getVoltage() : 0.0;
    float soc   = lipoFound ? lipo.getSOC() : 0.0;

    // Battery Bluetooth Send
    if (bleGamepad.isConnected() && lipoFound) {
      bleGamepad.setBatteryLevel((uint8_t)constrain(soc, 0.0f, 100.0f));
    }

    // 5. Display Refresh (each ~200 ms)
    static uint32_t lastOled = 0;
    if (oledFound && (now - lastOled >= 200)) {
      lastOled = now;

      display.clearDisplay();
      display.setCursor(0, 0);

      // Line 1: Battery & Status BLE Connection
      display.printf("%.2fV %.0f%% [%s]\n", vCell, soc, bleGamepad.isConnected() ? "CON" : "ADV");

      // Lines 2 & 3: Axis
      display.printf("L: %d, %d\n", rawLX, rawLY);
      display.printf("R: %d, %d\n", rawRX, rawRY);

      // Line 4: Buttons pressed
      display.printf("PRESSED (%d):\n", pressedCount);
      display.println(pressedNames.length() > 0 ? pressedNames : "- NONE -");

      display.display();
    }
  }
}
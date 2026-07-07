#include <Arduino.h>
#include <BleGamepad.h>

// Action Buttons
#define A_BUTTON 23        // A
#define B_BUTTON 22        // B
#define X_BUTTON 21        // X
#define Y_BUTTON 19        // Y

// Triggers & Shoulders
#define L1_BUTTON 18
#define L2_BUTTON 17
#define R1_BUTTON 16
#define R2_BUTTON 15

// Joystick Buttons
#define L3_BUTTON 14       // Left Joystick Press
#define R3_BUTTON 13       // Right Joystick Press

// Options Button
#define OPTIONS_BUTTON 12  // Options Button (Start/Select)

// D-pad (Directional Pad as buttons)
#define DPAD_UP 5
#define DPAD_DOWN 25
#define DPAD_LEFT 26
#define DPAD_RIGHT 27

// Joysticks
#define LEFT_VRX_JOYSTICK 34 // Analog input for Left Joystick X-axis
#define LEFT_VRY_JOYSTICK 35 // Analog input for Left Joystick Y-axis
#define RIGHT_VRX_JOYSTICK 32 // Analog input for Right Joystick X-axis
#define RIGHT_VRY_JOYSTICK 33 // Analog input for Right Joystick Y-axis
const int deadzone = 100; // Adjust deadzone as needed
const int numberOfSamples = 5; // Number of samples for averaging
float alpha = 0.1; // Low-pass filter smoothing factor

#define NUM_BUTTONS 15

int buttonsPins[NUM_BUTTONS] = {A_BUTTON, B_BUTTON, X_BUTTON, Y_BUTTON,
                                R1_BUTTON, R2_BUTTON, L1_BUTTON, L2_BUTTON,
                                OPTIONS_BUTTON, L3_BUTTON, R3_BUTTON,
                                DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT
                               };

// Button mappings for PC mode
int PCGamepadButtons[NUM_BUTTONS] = {1, 2, 3, 4,
                                     6, 8, 5, 7,
                                     10, 11, 12,
                                     13, 14, 15, 16,
                                    };

// Joystick values
int16_t leftVrxJoystickValue = 0;
int16_t leftVryJoystickValue = 0;
int16_t rightVrxJoystickValue = 0;
int16_t rightVryJoystickValue = 0;

// Initialize filtered values
float filteredLeftVrx = 0;
float filteredLeftVry = 0;
float filteredRightVrx = 0;
float filteredRightVry = 0;

// BLE Gamepad setup
BleGamepad bleGamepad("ESP32 Controller", "Robitza Electrical Engineer");
BleGamepadConfiguration bleGamepadConfig;

void setup() {
  delay(1000);
  Serial.begin(115200);

  // Set pins as inputs
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonsPins[i], INPUT_PULLUP);
  }

  // BLE Gamepad configuration
  bleGamepadConfig.setAutoReport(false);
  bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD);

  // Set PS5 Vendor ID (VID) and Product ID (PID)
  bleGamepadConfig.setVid(0x054C);  // PS5 VID
  bleGamepadConfig.setPid(0x0CE6);  // PS5 PID

  // Start the BLE Gamepad service with the configured settings
  bleGamepad.begin(&bleGamepadConfig);
}

void loop() {
  if (bleGamepad.isConnected()) {
    // Read and average joystick values
    int totalLeftVrx = 0;
    int totalLeftVry = 0;
    int totalRightVrx = 0;
    int totalRightVry = 0;

    for (int i = 0; i < numberOfSamples; i++) {
      totalLeftVrx += analogRead(LEFT_VRX_JOYSTICK);
      totalLeftVry += analogRead(LEFT_VRY_JOYSTICK);
      totalRightVrx += analogRead(RIGHT_VRX_JOYSTICK);
      totalRightVry += analogRead(RIGHT_VRY_JOYSTICK);
      delay(5); // Short delay between readings
    }

    // Calculate average values
    int avgLeftVrx = totalLeftVrx / numberOfSamples;
    int avgLeftVry = totalLeftVry / numberOfSamples;
    int avgRightVrx = totalRightVrx / numberOfSamples;
    int avgRightVry = totalRightVry / numberOfSamples;

    // Low-pass filter and mapping without deadzone
    // Left Joystick Horizontal
    filteredLeftVrx = (alpha * avgLeftVrx) + ((1 - alpha) * filteredLeftVrx);
    leftVrxJoystickValue = map(filteredLeftVrx, 0, 4095, 0, 1 * 32737) - (0 * 32737/2); // Adjust center as needed

    // Left Joystick Vertical
    filteredLeftVry = (alpha * avgLeftVry) + ((1 - alpha) * filteredLeftVry);
    leftVryJoystickValue = map(filteredLeftVry, 0, 4095, 0, 1 * 32737) - (0 * 32737/2); // Adjust center as needed
      
    // Right Joystick Horizontal
    filteredRightVrx = (alpha * avgRightVrx) + ((1 - alpha) * filteredRightVrx);
    rightVrxJoystickValue = map(filteredRightVrx, 0, 4095, 0, 1 * 32737) - (0 * 32737/2); // Adjust center as needed

    // Right Joystick Vertical
    filteredRightVry = (alpha * avgRightVry) + ((1 - alpha) * filteredRightVry);
    rightVryJoystickValue = map(filteredRightVry, 0, 4095, 0, 1 * 32737) - (0 * 32737/2); // Adjust center as needed

    // Handle buttons including D-pad
    for (int i = 0; i < NUM_BUTTONS; i++) {
      if (!digitalRead(buttonsPins[i])) {
        bleGamepad.press(PCGamepadButtons[i]);
      } else {
        bleGamepad.release(PCGamepadButtons[i]);
      }
    }

    // Set joystick axes for PC
    bleGamepad.setX(leftVrxJoystickValue);
    bleGamepad.setY(leftVryJoystickValue);
    bleGamepad.setZ(rightVrxJoystickValue);
    bleGamepad.setRX(rightVryJoystickValue);

    // Send the updated state to the PC
    bleGamepad.sendReport();
  }
}

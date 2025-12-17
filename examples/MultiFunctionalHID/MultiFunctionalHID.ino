/*
 * ESP32-C3 Multi-functional HID Device Example
 *
 * This example demonstrates how to use the ESP32-C3 as a combined
 * Controller, keyboard, and mouse over a single BLE connection.
 *
 * Features:
 * - Controller functionality (buttons, joysticks, triggers)
 * - Keyboard functionality (key presses, text input, modifiers)
 * - Mouse functionality (clicks, movement, scroll wheel)
 * - All combined in one HID device descriptor
 * - NEW: Configurable device types and options
 *
 * Hardware Requirements:
 * - ESP32-C3 development board
 * - Optional: Buttons, potentiometers, or other input devices
 *
 * Instructions:
 * 1. Upload this code to your ESP32-C3
 * 2. Pair with a computer/phone via Bluetooth
 * 3. The device will appear as a multi-functional HID device
 * 4. Use the functions to send Controller, keyboard, or mouse inputs
 *
 * Author: Enhanced ESP32-BLE-Controller Library
 * License: Same as original library
 */

#include <BleController.h>
#include "BleKeyboardKeys.h"

// Create BleController instance with custom name
BleController BleController("ESP32-C3 Multi-HID", "Espressif Systems");

// Demo button pins (adjust according to your hardware)
#define CONTROLLER_BUTTON_PIN 2  // Button for Controller demo
#define KEYBOARD_BUTTON_PIN 3 // Button for keyboard demo
#define MOUSE_BUTTON_PIN 4    // Button for mouse demo

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32-C3 Multi-functional HID Device");

  // Initialize buttons with pull-up resistors
  pinMode(CONTROLLER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(KEYBOARD_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MOUSE_BUTTON_PIN, INPUT_PULLUP);

  // Configure Controller settings (optional)
  BleControllerConfiguration BleControllerConfig;
  BleControllerConfig.setAutoReport(false); // Don't auto-send reports
  BleControllerConfig.setButtonCount(16);   // 16 buttons
  BleControllerConfig.setHatSwitchCount(2); // 2 hat switches
  BleControllerConfig.setAxesMax(32767);    // 16-bit axes resolution
  BleControllerConfig.setAxesMin(-32767);
  
  // NEW: Configure which devices are enabled (default: all)
  // BleControllerConfig.setEnabledDevices(DEVICE_ALL);  // All devices (default)
  // Or use individual setters:
  BleControllerConfig.setGamepadEnabled(true);   // Enable gamepad
  BleControllerConfig.setKeyboardEnabled(true);  // Enable keyboard  
  BleControllerConfig.setMouseEnabled(true);     // Enable mouse
  
  // NEW: Configure keyboard options
  BleControllerConfig.setKeyboardKeyCount(6);    // 6 simultaneous keys (default)
  
  // NEW: Configure mouse options
  BleControllerConfig.setMouseButtonCount(5);    // 5 buttons (left, right, middle, back, forward)
  BleControllerConfig.setMouseWheelEnabled(true);  // Enable vertical scroll
  BleControllerConfig.setMouseHWheelEnabled(true); // Enable horizontal scroll

  // Begin the BLE Controller with multi-HID support
  BleController.begin(&BleControllerConfig);

  Serial.println("Multi-HID device started!");
  Serial.println("Waiting for BLE connection...");
}

void loop() {
  if (BleController.isConnected()) {
    // Demo 1: Controller functionality
    demoController();
    delay(100);

    // Demo 2: Keyboard functionality
    demoKeyboard();
    delay(100);

    // Demo 3: Mouse functionality
    demoMouse();
    delay(100);

    // Combined demo
    demoCombined();
    delay(1000);
  } else {
    Serial.println("Waiting for BLE connection...");
    delay(2000);
  }
}

void demoController() {
  static unsigned long lastControllerDemo = 0;

  if (millis() - lastControllerDemo > 2000) { // Every 2 seconds
    Serial.println("Controller Demo: Button press and joystick movement");

    // Press Controller button 1
    BleController.press(BUTTON_1);
    BleController.sendReport();
    delay(100);

    // Move left joystick
    BleController.setLeftThumb(16000, -8000);
    BleController.sendReport();
    delay(100);

    // Release button and center joystick
    BleController.release(BUTTON_1);
    BleController.setLeftThumb(0, 0);
    BleController.sendReport();

    lastControllerDemo = millis();
  }
}

void demoKeyboard() {
  static unsigned long lastKeyboardDemo = 0;
  static int keyboardStep = 0;

  if (millis() - lastKeyboardDemo > 3000) { // Every 3 seconds
    Serial.println("Keyboard Demo: Typing text");

    switch (keyboardStep) {
    case 0:
      // Type "Hello World!"
      BleController.keyboardPrint("Hello World!");
      break;

    case 1:
      // Press Enter key
      BleController.keyboardPress(KEY_RETURN);
      delay(10);
      BleController.keyboardRelease(KEY_RETURN);
      break;

    case 2:
      // Type with modifier (Ctrl+A to select all)
      BleController.setKeyboardModifiers(KEY_MOD_LCTRL);
      BleController.keyboardPress(0x04); // 'A' key
      delay(10);
      BleController.keyboardReleaseAll();
      break;

    case 3:
      // Type replacement text
      BleController.keyboardPrint("ESP32-C3 Multi-HID Device!");
      break;
    }

    keyboardStep = (keyboardStep + 1) % 4;
    lastKeyboardDemo = millis();
  }
}

void demoMouse() {
  static unsigned long lastMouseDemo = 0;
  static int mouseStep = 0;

  if (millis() - lastMouseDemo > 4000) { // Every 4 seconds
    Serial.println("Mouse Demo: Click and movement");

    switch (mouseStep) {
    case 0:
      // Left click
      BleController.mouseClick(MOUSE_LEFT);
      Serial.println("  Left click");
      break;

    case 1:
      // Move mouse in a small circle
      for (int angle = 0; angle < 360; angle += 30) {
        int x = (int)(20 * cos(angle * PI / 180));
        int y = (int)(20 * sin(angle * PI / 180));
        BleController.mouseMove(x, y);
        delay(50);
      }
      Serial.println("  Mouse circle movement");
      break;

    case 2:
      // Right click
      BleController.mouseClick(MOUSE_RIGHT);
      Serial.println("  Right click");
      break;

    case 3:
      // Scroll wheel
      BleController.mouseScroll(3); // Scroll up
      delay(100);
      BleController.mouseScroll(-3); // Scroll down
      Serial.println("  Mouse scroll");
      break;
    }

    mouseStep = (mouseStep + 1) % 4;
    lastMouseDemo = millis();
  }
}

void demoCombined() {
  static unsigned long lastCombinedDemo = 0;

  if (millis() - lastCombinedDemo > 10000) { // Every 10 seconds
    Serial.println("Combined Demo: Controller + Keyboard + Mouse");

    // Gaming scenario: Press Controller button while typing in chat
    BleController.press(BUTTON_1); // Hold Controller button
    BleController.sendReport();

    delay(100);

    BleController.keyboardPrint("GG! "); // Type in chat

    delay(100);

    BleController.mouseClick(MOUSE_LEFT); // Click to send message

    delay(100);

    BleController.release(BUTTON_1); // Release Controller button
    BleController.sendReport();

    Serial.println("  Combined action completed");
    lastCombinedDemo = millis();
  }
}

// Hardware button handlers (if using physical buttons)
void handleHardwareButtons() {
  // Controller button
  if (digitalRead(CONTROLLER_BUTTON_PIN) == LOW) {
    BleController.press(BUTTON_1);
    BleController.sendReport();
    delay(50); // Debounce
  } else {
    BleController.release(BUTTON_1);
    BleController.sendReport();
  }

  // Keyboard button (types 'A')
  static bool keyboardButtonPressed = false;
  if (digitalRead(KEYBOARD_BUTTON_PIN) == LOW && !keyboardButtonPressed) {
    BleController.keyboardWrite(0x04); // 'A' key
    keyboardButtonPressed = true;
    delay(50); // Debounce
  } else if (digitalRead(KEYBOARD_BUTTON_PIN) == HIGH) {
    keyboardButtonPressed = false;
  }

  // Mouse button (left click)
  static bool mouseButtonPressed = false;
  if (digitalRead(MOUSE_BUTTON_PIN) == LOW && !mouseButtonPressed) {
    BleController.mouseClick(MOUSE_LEFT);
    mouseButtonPressed = true;
    delay(50); // Debounce
  } else if (digitalRead(MOUSE_BUTTON_PIN) == HIGH) {
    mouseButtonPressed = false;
  }
}

// Advanced examples for specific use cases

void gamingKeyboardMacro() {
  // Gaming macro: Ctrl+Shift+F1 (common for game overlays)
  BleController.setKeyboardModifiers(KEY_MOD_LCTRL | KEY_MOD_LSHIFT);
  BleController.keyboardPress(KEY_F1);
  delay(10);
  BleController.keyboardReleaseAll();
}

void mediaControls() {
  // Media control example (would need to be added to HID descriptor)
  // This is just an example of how you might extend functionality
  Serial.println("Media controls would go here (needs descriptor extension)");
}

void customHIDReport() {
  // Example of sending custom raw HID reports
  uint8_t customControllerReport[] = {0x01, 0xFF, 0x00, 0x7F, 0x7F,
                                   0x00, 0x00, 0x00, 0x00};
  BleController.rawAction(customControllerReport, sizeof(customControllerReport));

  uint8_t customKeyboardReport[] = {0x02, 0x02, 0x00, 0x04, 0x00,
                                    0x00, 0x00, 0x00, 0x00}; // Shift+A
  BleController.rawKeyboardAction(customKeyboardReport,
                               sizeof(customKeyboardReport));

  uint8_t customMouseReport[] = {0x03, 0x01, 0x10, 0x10,
                                 0x00}; // Left click + move
  BleController.rawMouseAction(customMouseReport, sizeof(customMouseReport));
}

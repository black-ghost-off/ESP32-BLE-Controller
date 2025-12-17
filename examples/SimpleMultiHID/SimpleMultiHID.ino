/*
 * Simple Multi-HID Example
 *
 * This example shows basic usage of Controller, keyboard, and mouse
 * functionality on ESP32-C3 with a single BLE connection.
 *
 * The device will cycle through different input types every few seconds.
 * 
 * NEW: You can now configure which devices are enabled!
 * See ConfigurableMultiHID example for advanced configuration.
 */

#include <BleController.h>

BleController bleDevice("ESP32-C3 Multi-Device", "Espressif");
BleControllerConfiguration config;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-C3 Multi-functional HID Device");

  // Optional: Configure which devices to enable (default is all)
  // Uncomment lines below to customize:
  // config.setGamepadEnabled(true);   // Enable gamepad
  // config.setKeyboardEnabled(true);  // Enable keyboard
  // config.setMouseEnabled(true);     // Enable mouse
  
  // Optional: Configure keyboard (1-6 simultaneous keys)
  // config.setKeyboardKeyCount(6);
  
  // Optional: Configure mouse
  // config.setMouseButtonCount(5);    // 1-5 buttons
  // config.setMouseWheelEnabled(true);
  // config.setMouseHWheelEnabled(true);

  // Start the multi-HID device
  bleDevice.begin(&config);
  Serial.println("Waiting for Bluetooth connection...");
}

void loop() {
  if (bleDevice.isConnected()) {
    Serial.println("Connected! Running demos...");

    // Controller demo
    Serial.println("1. Controller: Pressing button 1");
    bleDevice.press(BUTTON_1);
    bleDevice.sendReport();
    delay(500);
    bleDevice.release(BUTTON_1);
    bleDevice.sendReport();
    delay(1000);

    // Keyboard demo
    Serial.println("2. Keyboard: Typing 'Hello'");
    bleDevice.keyboardPrint("Hello ");
    delay(1000);

    // Mouse demo
    Serial.println("3. Mouse: Left click");
    bleDevice.mouseClick(MOUSE_LEFT);
    delay(1000);

    // Combined demo
    Serial.println("4. Combined: Controller button + typing");
    bleDevice.press(BUTTON_2); // Hold Controller button
    bleDevice.sendReport();
    bleDevice.keyboardPrint("Gaming!"); // Type text
    delay(500);
    bleDevice.release(BUTTON_2); // Release button
    bleDevice.sendReport();

    delay(3000); // Wait before next cycle

  } else {
    Serial.println("Not connected, waiting...");
    delay(2000);
  }
}

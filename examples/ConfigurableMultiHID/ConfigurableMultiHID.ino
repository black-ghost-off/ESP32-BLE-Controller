/*
 * Configurable Multi-HID Example
 *
 * This example demonstrates how to configure which HID devices are enabled
 * (Gamepad, Keyboard, Mouse) and their specific options.
 *
 * Features demonstrated:
 * - Enable/disable individual devices (Gamepad, Keyboard, Mouse)
 * - Configure keyboard key count (simultaneous keys)
 * - Configure mouse button count and wheel options
 * - Create custom device combinations (e.g., Mouse + Keyboard only)
 *
 * Use Cases:
 * - Gamepad Only: For dedicated game controllers
 * - Keyboard + Mouse: For remote desktop or presentation control
 * - Mouse Only: For simple pointer/clicker devices
 * - All Devices: Full multi-functional HID device
 *
 * Hardware Requirements:
 * - ESP32-C3 or compatible ESP32 board with BLE support
 */

#include <Arduino.h>
#include <BleController.h>

// Create BleController instance
BleController bleController("Configurable Multi-HID", "Espressif");

// Configuration object
BleControllerConfiguration config;

// Uncomment ONE of the following configuration blocks:

// ============ CONFIGURATION 1: All Devices Enabled (Default) ============
void configureAllDevices() {
  Serial.println("Configuration: All Devices (Gamepad + Keyboard + Mouse)");
  
  // Enable all devices (this is the default)
  config.setEnabledDevices(DEVICE_ALL);
  // Or individually:
  // config.setGamepadEnabled(true);
  // config.setKeyboardEnabled(true);
  // config.setMouseEnabled(true);
  
  // Gamepad settings
  config.setButtonCount(16);          // 16 gamepad buttons
  config.setHatSwitchCount(1);        // 1 D-pad
  config.setWhichAxes(true, true, true, true, true, true, false, false); // X,Y,Z,RX,RY,RZ
  
  // Keyboard settings
  config.setKeyboardKeyCount(6);      // 6 simultaneous keys (standard)
  
  // Mouse settings
  config.setMouseButtonCount(5);      // 5 buttons (left, right, middle, back, forward)
  config.setMouseWheelEnabled(true);  // Enable vertical scroll
  config.setMouseHWheelEnabled(true); // Enable horizontal scroll
}

// ============ CONFIGURATION 2: Mouse + Keyboard Only ============
void configureMouseKeyboard() {
  Serial.println("Configuration: Mouse + Keyboard Only");
  
  // Disable gamepad, enable keyboard and mouse
  config.setGamepadEnabled(false);
  config.setKeyboardEnabled(true);
  config.setMouseEnabled(true);
  
  // Keyboard settings
  config.setKeyboardKeyCount(6);      // 6 simultaneous keys
  
  // Mouse settings
  config.setMouseButtonCount(3);      // 3 buttons (left, right, middle)
  config.setMouseWheelEnabled(true);  // Enable scroll
  config.setMouseHWheelEnabled(false); // Disable horizontal scroll
}

// ============ CONFIGURATION 3: Gamepad Only ============
void configureGamepadOnly() {
  Serial.println("Configuration: Gamepad Only");
  
  // Enable only gamepad
  config.setGamepadEnabled(true);
  config.setKeyboardEnabled(false);
  config.setMouseEnabled(false);
  
  // Gamepad settings
  config.setButtonCount(16);
  config.setHatSwitchCount(2);        // 2 D-pads
  config.setWhichAxes(true, true, true, true, true, true, true, true); // All axes including sliders
  config.setIncludeStart(true);
  config.setIncludeSelect(true);
}

// ============ CONFIGURATION 4: Simple Mouse Only ============
void configureSimpleMouse() {
  Serial.println("Configuration: Simple Mouse Only");
  
  // Enable only mouse
  config.setGamepadEnabled(false);
  config.setKeyboardEnabled(false);
  config.setMouseEnabled(true);
  
  // Minimal mouse - just left click and movement
  config.setMouseButtonCount(1);      // Only left click
  config.setMouseWheelEnabled(false); // No scroll wheel
  config.setMouseHWheelEnabled(false);
}

// ============ CONFIGURATION 5: Presentation Remote ============
void configurePresentationRemote() {
  Serial.println("Configuration: Presentation Remote (Keyboard + Mouse)");
  
  // Keyboard for page navigation, mouse for pointer
  config.setGamepadEnabled(false);
  config.setKeyboardEnabled(true);
  config.setMouseEnabled(true);
  
  // Minimal keyboard - just for arrow keys and space
  config.setKeyboardKeyCount(2);      // Only 2 keys needed at once
  
  // Simple mouse
  config.setMouseButtonCount(1);      // Left click only
  config.setMouseWheelEnabled(false);
  config.setMouseHWheelEnabled(false);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nStarting Configurable Multi-HID Device");
  
  // ==================================================
  // SELECT YOUR CONFIGURATION HERE
  // Uncomment ONE of the following lines:
  // ==================================================
  configureAllDevices();           // Default - all devices
  // configureMouseKeyboard();     // Mouse + Keyboard only
  // configureGamepadOnly();       // Gamepad only
  // configureSimpleMouse();       // Simple mouse only
  // configurePresentationRemote(); // Presentation remote
  
  // Common settings
  config.setAutoReport(false);        // Manual report sending
  config.setVid(0xe502);
  config.setPid(0xbbab);
  
  // Start the BLE controller with our configuration
  bleController.begin(&config);
  
  Serial.println("BLE Device started!");
  Serial.println("Waiting for connection...");
  
  // Print configuration summary
  Serial.println("\n--- Configuration Summary ---");
  Serial.print("Gamepad: ");
  Serial.println(config.getGamepadEnabled() ? "ENABLED" : "DISABLED");
  Serial.print("Keyboard: ");
  Serial.println(config.getKeyboardEnabled() ? "ENABLED" : "DISABLED");
  Serial.print("Mouse: ");
  Serial.println(config.getMouseEnabled() ? "ENABLED" : "DISABLED");
  
  if (config.getKeyboardEnabled()) {
    Serial.print("  Keyboard Keys: ");
    Serial.println(config.getKeyboardKeyCount());
  }
  
  if (config.getMouseEnabled()) {
    Serial.print("  Mouse Buttons: ");
    Serial.println(config.getMouseButtonCount());
    Serial.print("  Scroll Wheel: ");
    Serial.println(config.getMouseWheelEnabled() ? "YES" : "NO");
    Serial.print("  Horizontal Scroll: ");
    Serial.println(config.getMouseHWheelEnabled() ? "YES" : "NO");
  }
  Serial.println("-----------------------------\n");
}

void loop() {
  if (bleController.isConnected()) {
    static unsigned long lastDemo = 0;
    
    if (millis() - lastDemo > 3000) {
      lastDemo = millis();
      
      Serial.println("Connected! Running demo...");
      
      // Demo gamepad if enabled
      if (config.getGamepadEnabled()) {
        Serial.println("  Gamepad: Press Button 1");
        bleController.press(BUTTON_1);
        bleController.sendReport();
        delay(100);
        bleController.release(BUTTON_1);
        bleController.sendReport();
      }
      
      // Demo keyboard if enabled
      if (config.getKeyboardEnabled()) {
        Serial.println("  Keyboard: Type 'Hi'");
        bleController.keyboardPrint("Hi ");
      }
      
      // Demo mouse if enabled
      if (config.getMouseEnabled()) {
        Serial.println("  Mouse: Move and click");
        bleController.mouseMove(10, 5);
        delay(50);
        bleController.mouseClick(MOUSE_LEFT);
      }
    }
  } else {
    static unsigned long lastMsg = 0;
    if (millis() - lastMsg > 5000) {
      lastMsg = millis();
      Serial.println("Waiting for BLE connection...");
    }
  }
  
  delay(10);
}

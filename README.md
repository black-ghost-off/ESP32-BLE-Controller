# ESP32-BLE-Controller

Bassed on [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad)

## License
Published under the MIT license. Please see license.txt.

## Features

 - [x] Button press (128 buttons)
 - [x] Button release (128 buttons)
 - [x] Axes movement (6 axes (configurable resolution up to 16 bit) (x, y, z, rX, rY, rZ) --> In Windows usually (Left Thumb X, Left Thumb Y, Right Thumb X, Left Trigger, Right Trigger, Right Thumb Y))
 - [x] Gyroscope and Accelerometer
 - [x] Set battery percentage
 - [x] Set battery power state information using UUID 0x2A1A. Use nRF Connect on Android for example to see this information
 - [x] 2 Sliders (configurable resolution up to 16 bit) (Slider 1 and Slider 2)
 - [x] 4 point of view hats (ie. d-pad plus 3 other hat switches)
 - [x] Simulation controls (rudder, throttle, accelerator, brake, steering)
 - [x] Special buttons (start, select, menu, home, back, volume up, volume down, volume mute) all disabled by default
 - [x] Configurable HID descriptor
 - [x] Configurable VID and PID values
 - [x] Configurable BLE characteristics (name, manufacturer, model number, software revision, serial number, firmware revision, hardware revision)	
 - [x] Report optional battery level to host
 - [x] Uses efficient NimBLE bluetooth library
 - [x] Output report function
 - [x] Functions available for force pairing/ignore current client and/or delete pairings
 - [x] Nordic UART Service functionality at same time as Controller. See examples
 - [x] Multi-functional HID device support (Controller + Keyboard + Mouse in single BLE connection)
 - [x] **Configurable device types** - Enable/disable Gamepad, Keyboard, Mouse independently
 - [x] **Configurable keyboard** - Set number of simultaneous keys (1-6)
 - [x] **Configurable mouse** - Set button count (1-5), enable/disable scroll wheels
 - [x] Keyboard functionality (key press/release, text input, modifier keys, function keys)
 - [x] Mouse functionality (left/right/middle click, movement, scroll wheel)
 - [x] Raw HID report support for keyboard and mouse
 - [x] Compatible with Windows
 - [x] Compatible with Android (Android OS maps default buttons / axes / hats slightly differently than Windows) (see notes)
 - [x] Compatible with Linux (limited testing)
 - [x] Compatible with MacOS X (limited testing)
 - [ ] Compatible with iOS (No - not even for accessibility switch - This is not a “Made for iPhone” (MFI) compatible device)
                           (Use the Xinput fork suggested below which has been tested to work)

## NimBLE
Since version 3 of this library, the more efficient NimBLE library is used instead of the default BLE implementation
Please use the library manager to install it, or get it from here: https://github.com/h2zero/NimBLE-Arduino
Since version 3, this library also supports a configurable HID desciptor, which allows users to customise how the device presents itself to the OS (number of buttons, hats, axes, sliders, simulation controls etc).
See the examples for guidance.

## POSSIBLE BREAKING CHANGES - PLEASE READ
A large code rebase (configuration class) along with some extra features (start, select, menu, home, back, volume up, volume down and volume mute buttons) has been committed thanks to @dexterdy

Since version 5 of this library, the axes and simulation controls have configurable min and max values
The decision was made to set defaults to 0 for minimum and 32767 for maximum (previously -32767 to 32767)
This was due to the fact that non-Windows operating systems and some online web-based game controller testers didn't play well with negative numbers. Existing sketches should take note, and see the DrivingControllerTest example for how to set back to -32767 if wanted

This version endeavors to be compatible with the latest released version of NimBLE-Arduino through the Arduino Library Manager; currently version 2.2.1 at the time of this writing; --> https://github.com/h2zero/NimBLE-Arduino/releases/tag/2.2.1

setAxes accepts axes in the order (x, y, z, rx, ry, rz)
setHIDAxes accepts them in the order (x, y, z, rz, rx, ry)

Please see updated examples

## Installation
- (Make sure you can use the ESP32 with the Arduino IDE. [Instructions can be found here.](https://github.com/espressif/arduino-esp32#installation-instructions))
- [Download the latest release of this library from the release page.](https://github.com/black-ghost-off/ESP32-BLE-Controller/releases)
- In the Arduino IDE go to "Sketch" -> "Include Library" -> "Add .ZIP Library..." and select the file you just downloaded.
- In the Arduino IDE go to "Tools" -> "Manage Libraries..." -> Filter for "NimBLE-Arduino" by h2zero and install.
- You can now go to "File" -> "Examples" -> "ESP32 BLE Controller" and select an example to get started.

## Example

``` C++
/*
 * This example turns the ESP32 into a Bluetooth LE Controller that presses buttons and moves axis
 *
 * At the moment we are using the default settings, but they can be canged using a BleControllerConfig instance as parameter for the begin function.
 *
 * Possible buttons are:
 * BUTTON_1 through to BUTTON_16
 * (16 buttons by default. Library can be configured to use up to 128)
 *
 * Possible DPAD/HAT switch position values are:
 * DPAD_CENTERED, DPAD_UP, DPAD_UP_RIGHT, DPAD_RIGHT, DPAD_DOWN_RIGHT, DPAD_DOWN, DPAD_DOWN_LEFT, DPAD_LEFT, DPAD_UP_LEFT
 * (or HAT_CENTERED, HAT_UP etc)
 *
 * BleController.setAxes sets all axes at once. There are a few:
 * (x axis, y axis, z axis, rx axis, ry axis, rz axis, slider 1, slider 2)
 *
 * Alternatively, BleController.setHIDAxes sets all axes at once. in the order of:
 * (x axis, y axis, z axis, rz axis, ry axis, rz axis, slider 1, slider 2)  <- order HID report is actually given in
 *
 * Library can also be configured to support up to 5 simulation controls
 * (rudder, throttle, accelerator, brake, steering), but they are not enabled by default.
 *
 * Library can also be configured to support different function buttons
 * (start, select, menu, home, back, volume increase, volume decrease, volume mute)
 * start and select are enabled by default
 */

#include <Arduino.h>
#include <BleController.h>

BleController BleController;

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting BLE work!");
    BleController.begin();
    // The default BleController.begin() above enables 16 buttons, all axes, one hat, and no simulation controls or special buttons
}

void loop()
{
    if (BleController.isConnected())
    {
        Serial.println("Press buttons 5, 16 and start. Move all enabled axes to max. Set DPAD (hat 1) to down right.");
        BleController.press(BUTTON_5);
        BleController.press(BUTTON_16);
        BleController.pressStart();
        BleController.setAxes(32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767);       //(X, Y, Z, RX, RY, RZ)
        //BleController.setHIDAxes(32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767);  //(X, Y, Z, RZ, RX, RY)
        BleController.setHat1(HAT_DOWN_RIGHT);
        // All axes, sliders, hats etc can also be set independently. See the IndividualAxes.ino example
        delay(500);

        Serial.println("Release button 5 and start. Move all axes to min. Set DPAD (hat 1) to centred.");
        BleController.release(BUTTON_5);
        BleController.releaseStart();
        BleController.setHat1(HAT_CENTERED);
        BleController.setAxes(0, 0, 0, 0, 0, 0, 0, 0);           //(X, Y, Z, RX, RY, RZ)
        //BleController.setHIDAxes(0, 0, 0, 0, 0, 0, 0, 0);      //(X, Y, Z, RZ, RX, RY)
        delay(500);
    }
}

```
By default, reports are sent on every button press/release or axis/slider/hat/simulation movement, however this can be disabled, and then you manually call sendReport on the Controller instance as shown in the IndividualAxes.ino example.

VID and PID values can be set. See TestAll.ino for example.

There is also Bluetooth specific information that you can use (optional):

Instead of `BleController BleController;` you can do `BleController BleController("Bluetooth Device Name", "Bluetooth Device Manufacturer", 100);`.
The third parameter is the initial battery level of your device.
By default the battery level will be set to 100%, the device name will be `ESP32 BLE Controller` and the manufacturer will be `Espressif`.

Battery level can be set during operation by calling, for example, BleController.setBatteryLevel(80);
Update sent on next Controller update if auto reporting is not enabled


## Troubleshooting Guide
Troubleshooting guide and suggestions can be found in [TroubleshootingGuide](TroubleshootingGuide.md)

## Credits
Credits to [T-vK](https://github.com/T-vK) as this library is based on his ESP32-BLE-Mouse library (https://github.com/T-vK/ESP32-BLE-Mouse) that he provided.

Credits to [chegewara](https://github.com/chegewara) as the ESP32-BLE-Mouse library is based on [this piece of code](https://github.com/nkolban/esp32-snippets/issues/230#issuecomment-473135679) that he provided.

Credits to [wakwak-koba](https://github.com/wakwak-koba) for the NimBLE [code](https://github.com/wakwak-koba/ESP32-NimBLE-Controller) that he provided.

## Notes
This library allows you to make the ESP32 act as a Bluetooth Controller and control what it does.  
Relies on [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)

Use [this](http://www.planetpointy.co.uk/joystick-test-application/) Windows test app to test/see all of the buttons
Ensure you have Direct X 9 installed

Controllers desgined for Android use a different button mapping. This effects analog triggers, where the standard left and right trigger axes are not detected.
Android calls the HID report for right trigger `"GAS"` and left trigger `"BRAKE"`. Enabling the `"Accelerator"` and `"Brake"` simulation controls allows them to be used instead of right and left trigger.

Right thumbstick on Windows is usually z, rz, whereas on Android, this may be z, rx, so you may want to set them separately with setZ and setRX, instead of using setRightThumb(z, rz), or use setRightThumbAndroid(z, rx)

## Multi-functional HID Device Support

Starting from this version, the library supports creating a **multi-functional HID device** that combines Controller, keyboard, and mouse functionality in a single BLE connection. This means your ESP32-C3 can act as all three input devices simultaneously.

### Features:
- **Controller**: All existing Controller functionality (buttons, axes, triggers, hats, etc.)
- **Keyboard**: Full keyboard support with modifier keys, function keys, and text input
- **Mouse**: Mouse buttons (left, right, middle), movement, and scroll wheel
- **Combined Usage**: Use all three input types at the same time

### Keyboard Functions:
```cpp
// Basic key operations
BleController.keyboardPress(KEY_A);        // Press 'A' key
BleController.keyboardRelease(KEY_A);      // Release 'A' key
BleController.keyboardWrite(KEY_A);        // Press and release 'A' key
BleController.keyboardReleaseAll();        // Release all keys

// Text input
BleController.keyboardPrint("Hello World!"); // Type text string
BleController.keyboardWrite("Text here");     // Alternative text input

// Modifier keys
BleController.setKeyboardModifiers(KEY_MOD_LCTRL | KEY_MOD_LSHIFT); // Ctrl+Shift
BleController.keyboardPress(KEY_A);        // Ctrl+Shift+A
BleController.keyboardReleaseAll();        // Release all

// Send raw keyboard report
uint8_t report[] = {0x02, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}; // Shift+A
BleController.rawKeyboardAction(report, sizeof(report));
```

### Mouse Functions:
```cpp
// Mouse buttons
BleController.mouseClick(MOUSE_LEFT);      // Left click
BleController.mouseClick(MOUSE_RIGHT);     // Right click
BleController.mouseClick(MOUSE_MIDDLE);    // Middle click
BleController.mousePress(MOUSE_LEFT);      // Hold left button
BleController.mouseRelease(MOUSE_LEFT);    // Release left button
BleController.mouseReleaseAll();           // Release all buttons

// Mouse movement
BleController.mouseMove(10, -5);           // Move right 10, up 5 pixels
BleController.mouseScroll(3);              // Scroll up 3 units
BleController.mouseScroll(-2);             // Scroll down 2 units

// Send raw mouse report
uint8_t report[] = {0x03, 0x01, 0x10, 0x10, 0x00}; // Left click + move
BleController.rawMouseAction(report, sizeof(report));
```

### Available Key Constants:
The library includes comprehensive key definitions in `BleKeyboardKeys.h`:
- **Modifier keys**: `KEY_LEFT_CTRL`, `KEY_LEFT_SHIFT`, `KEY_LEFT_ALT`, `KEY_LEFT_GUI`, etc.
- **Function keys**: `KEY_F1` through `KEY_F12`
- **Arrow keys**: `KEY_UP_ARROW`, `KEY_DOWN_ARROW`, `KEY_LEFT_ARROW`, `KEY_RIGHT_ARROW`
- **Navigation**: `KEY_BACKSPACE`, `KEY_TAB`, `KEY_RETURN`, `KEY_ESC`, `KEY_INSERT`, `KEY_DELETE`, etc.
- **System keys**: `KEY_CAPS_LOCK`, `KEY_PRINT_SCREEN`, `KEY_SCROLL_LOCK`, `KEY_PAUSE`, etc.
- **Keypad**: `KEY_KP_0` through `KEY_KP_9`, `KEY_KP_PLUS`, `KEY_KP_MINUS`, etc.

### Mouse Button Constants:
- `MOUSE_LEFT` - Left mouse button
- `MOUSE_RIGHT` - Right mouse button  
- `MOUSE_MIDDLE` - Middle mouse button (scroll wheel click)

### Combined Usage Example:
```cpp
#include <BleController.h>

BleController bleDevice("ESP32-C3 Multi-Device");

void setup() {
  bleDevice.begin();
}

void loop() {
  if (bleDevice.isConnected()) {
    // Use Controller
    bleDevice.press(BUTTON_1);
    bleDevice.setLeftThumb(16000, 8000);
    bleDevice.sendReport();
    
    // Use keyboard
    bleDevice.keyboardPrint("Hello from ESP32!");
    
    // Use mouse
    bleDevice.mouseClick(MOUSE_LEFT);
    bleDevice.mouseMove(50, 25);
    
    delay(1000);
  }
}
```

### Examples:
- `SimpleMultiHID.ino` - Basic multi-device functionality
- `MultiFunctionalHID.ino` - Advanced usage with all features demonstrated
- `ConfigurableMultiHID.ino` - **NEW** Configure which devices to enable and their options

This multi-HID functionality is particularly useful for:
- **Gaming applications** where you need Controller controls plus keyboard shortcuts
- **Accessibility devices** that combine multiple input methods
- **Remote control applications** with comprehensive input capabilities
- **Creative/productivity tools** that benefit from multiple input modalities

## Device Configuration

You can configure which HID devices are enabled and customize their settings. By default, all devices (Gamepad, Keyboard, Mouse) are enabled.

### Device Enable/Disable Constants:
```cpp
DEVICE_GAMEPAD   // Gamepad/Controller device
DEVICE_KEYBOARD  // Keyboard device
DEVICE_MOUSE     // Mouse device
DEVICE_ALL       // All devices (default)
```

### Device Configuration Example:
```cpp
BleControllerConfiguration config;

// Method 1: Using bitmask
config.setEnabledDevices(DEVICE_KEYBOARD | DEVICE_MOUSE);  // Only keyboard and mouse

// Method 2: Using individual setters
config.setGamepadEnabled(false);   // Disable gamepad
config.setKeyboardEnabled(true);   // Enable keyboard
config.setMouseEnabled(true);      // Enable mouse

bleController.begin(&config);
```

### Keyboard Configuration:
```cpp
// Set number of simultaneous keys (1-6, default: 6)
config.setKeyboardKeyCount(6);

// Check current setting
uint8_t keyCount = config.getKeyboardKeyCount();
```

### Mouse Configuration:
```cpp
// Set number of mouse buttons (1-5, default: 5)
// 1=left, 2=left+right, 3=left+right+middle, 5=all including back/forward
config.setMouseButtonCount(3);

// Enable/disable scroll wheel (default: enabled)
config.setMouseWheelEnabled(true);

// Enable/disable horizontal scroll (default: enabled)
config.setMouseHWheelEnabled(true);

// Check current settings
uint8_t buttonCount = config.getMouseButtonCount();
bool hasWheel = config.getMouseWheelEnabled();
bool hasHWheel = config.getMouseHWheelEnabled();
```

### Common Device Configurations:

**Gamepad Only (Game Controller):**
```cpp
config.setGamepadEnabled(true);
config.setKeyboardEnabled(false);
config.setMouseEnabled(false);
config.setButtonCount(16);
config.setHatSwitchCount(1);
```

**Keyboard + Mouse (Desktop Remote):**
```cpp
config.setGamepadEnabled(false);
config.setKeyboardEnabled(true);
config.setMouseEnabled(true);
config.setKeyboardKeyCount(6);
config.setMouseButtonCount(3);
config.setMouseWheelEnabled(true);
```

**Simple Mouse (Presentation Clicker):**
```cpp
config.setGamepadEnabled(false);
config.setKeyboardEnabled(false);
config.setMouseEnabled(true);
config.setMouseButtonCount(1);      // Left click only
config.setMouseWheelEnabled(false);
config.setMouseHWheelEnabled(false);
```

**All Devices (Multi-functional HID):**
```cpp
config.setEnabledDevices(DEVICE_ALL);  // or just use defaults
```


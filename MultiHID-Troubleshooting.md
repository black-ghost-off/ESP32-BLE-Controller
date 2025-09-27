# Multi-HID Device Troubleshooting Guide

## Common Issues and Solutions

### 1. Device Not Recognized as Multi-HID
**Symptoms**: Device only appears as gamepad, keyboard or mouse functions don't work
**Solutions**:
- Ensure you're using the updated library with multi-HID support
- Check that the HID descriptor includes all three device types
- Try unpairing and re-pairing the device
- Restart the ESP32-C3 and try again

### 2. Some Functions Not Working
**Symptoms**: Gamepad works but keyboard/mouse doesn't (or vice versa)
**Solutions**:
- Check that you're calling the correct send functions:
  - `sendReport()` for gamepad
  - `sendKeyboardReport()` for keyboard (called automatically by most keyboard functions)
  - `sendMouseReport()` for mouse (called automatically by most mouse functions)
- Verify the device is connected with `isConnected()`
- Check that the correct report IDs are being used

### 3. ESP32-C3 Specific Issues
**Symptoms**: Code compiles but doesn't work on ESP32-C3
**Solutions**:
- Ensure you're using ESP32-C3 compatible board definitions
- Check that NimBLE is properly configured for ESP32-C3
- Verify sufficient memory is available (the descriptor is larger for multi-HID)
- Make sure the task priority is set correctly (default is 1 for ESP32-C3)

### 4. Connection Issues
**Symptoms**: Device doesn't appear in Bluetooth settings or won't pair
**Solutions**:
- Clear Bluetooth cache on the host device
- Delete existing pairings using `deleteAllBonds()`
- Try `enterPairingMode()` to force pairing mode
- Check if the device name is too long (keep under 29 characters)

### 5. Input Lag or Missed Commands
**Symptoms**: Inputs are delayed or some inputs are ignored
**Solutions**:
- Reduce the frequency of reports being sent
- Use `setAutoReport(false)` and manually control when reports are sent
- Add small delays between different input types
- Check Bluetooth connection quality

### 6. Platform-Specific Issues

#### Windows:
- Install latest Bluetooth drivers
- Some older Windows versions may not support multi-HID devices properly
- Try using "Add a device" instead of automatic pairing

#### Android:
- Multi-HID support varies by Android version and manufacturer
- Some Android devices may only recognize the first HID interface
- Try different Android versions if possible

#### macOS:
- Multi-HID support is generally good
- May need to grant accessibility permissions for full functionality

#### Linux:
- Ensure `bluez` is up to date
- May need to manually configure HID support
- Check system logs for HID-related messages

## Debugging Tips

### 1. Enable Debug Output
```cpp
#define BLE_GAMEPAD_DEBUG 1
```
This will print HID reports to serial for debugging.

### 2. Test Individual Functions
Test each input type separately:
```cpp
// Test gamepad only
bleDevice.press(BUTTON_1);
bleDevice.sendReport();

// Test keyboard only  
bleDevice.keyboardWrite('A');

// Test mouse only
bleDevice.mouseClick(MOUSE_LEFT);
```

### 3. Monitor HID Reports
Use tools like:
- **Windows**: USB Device Tree Viewer, or device manager
- **Android**: nRF Connect app
- **Linux**: `hid-tools` or `evtest`
- **macOS**: Bluetooth Explorer (in Additional Tools for Xcode)

### 4. Check Memory Usage
Multi-HID uses more memory due to larger descriptors:
```cpp
Serial.println("Free heap: " + String(ESP.getFreeHeap()));
```

### 5. Verify Report IDs
Ensure the correct report IDs are being used:
- Gamepad: 0x01 (GAMEPAD_REPORT_ID)
- Keyboard: 0x02 (KEYBOARD_REPORT_ID)  
- Mouse: 0x03 (MOUSE_REPORT_ID)

## Advanced Troubleshooting

### Custom HID Descriptor Issues
If you modify the HID descriptor:
- Use online HID descriptor tools to validate syntax
- Test with a simple descriptor first
- Ensure report sizes match your data structures
- Verify logical min/max values are correct

### Performance Optimization
For better performance:
- Batch similar operations together
- Use raw report functions for high-frequency updates
- Consider using different connection intervals
- Optimize the order of operations in your loop

### Memory Issues
If running out of memory:
- Reduce buffer sizes where possible
- Disable unused features in BleControllerConfiguration
- Monitor heap fragmentation
- Consider using PSRAM if available

## Getting Help

If you're still having issues:
1. Check the GitHub issues for similar problems
2. Provide detailed information:
   - ESP32-C3 board model
   - Host device (OS and version)
   - Code snippet showing the problem
   - Serial output with debug enabled
3. Test with the provided examples first
4. Try with a different host device to isolate the issue

## Hardware Considerations

### ESP32-C3 Limitations
- Limited GPIO pins compared to other ESP32 variants
- Single core (may affect complex multi-tasking)
- Limited RAM (be mindful of memory usage)

### Power Management
- Multi-HID functionality may use more power
- Consider sleep modes for battery-powered applications
- Monitor current consumption during development

### Signal Quality
- Ensure good antenna design for reliable BLE connection
- Keep the device within reasonable range of the host
- Avoid interference from other 2.4GHz devices

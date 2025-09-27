/*
 * This example turns the ESP32 into a Bluetooth LE gamepad that presses buttons and moves axis
 *
 * Possible buttons are:
 * BUTTON_1 through to BUTTON_128 (Windows gamepad tester only visualises the first 32)
 ^ Use http://www.planetpointy.co.uk/joystick-test-application/ to visualise all of them
 * Whenever you adjust the amount of buttons/axes etc, make sure you unpair and repair the BLE device
 *
 * Possible DPAD/HAT switch position values are:
 * DPAD_CENTERED, DPAD_UP, DPAD_UP_RIGHT, DPAD_RIGHT, DPAD_DOWN_RIGHT, DPAD_DOWN, DPAD_DOWN_LEFT, DPAD_LEFT, DPAD_UP_LEFT
 *
 * BleController.setAxes takes the following int16_t parameters for the Left/Right Thumb X/Y, Left/Right Triggers plus slider1 and slider2:
 * (Left Thumb X, Left Thumb Y, Right Thumb X, Right Thumb Y, Left Trigger, Right Trigger, Slider 1, Slider 2) (x, y, z, rx, ry, rz)
 *
 * BleController.setHIDAxes instead takes them in a slightly different order (x, y, z, rz, rx, ry)
 *
 * BleController.setLeftThumb (or setRightThumb) takes 2 int16_t parameters for x and y axes (or z and rZ axes)
 * BleController.setRightThumbAndroid takes 2 int16_t parameters for z and rx axes
 *
 * BleController.setLeftTrigger (or setRightTrigger) takes 1 int16_t parameter for rX axis (or rY axis)
 *
 * BleController.setSlider1 (or setSlider2) takes 1 int16_t parameter for slider 1 (or slider 2)
 *
 * BleController.setHat1 takes a hat position as above (or 0 = centered and 1~8 are the 8 possible directions)
 *
 * setHats, setTriggers and setSliders functions are also available for setting all hats/triggers/sliders at once
 *
 * The example shows that you can set axes/hats independantly, or together.
 *
 * It also shows that you can disable the autoReport feature (enabled by default), and manually call the sendReport() function when wanted
 *
 */

#include <Arduino.h>
#include <BleController.h>

BleController BleController;

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting BLE work!");
    BleControllerConfiguration BleControllerConfig;
    BleControllerConfig.setAutoReport(false); // This is true by default
    BleControllerConfig.setButtonCount(128);
    BleControllerConfig.setHatSwitchCount(2);
    BleController.begin(&BleControllerConfig); // Creates a gamepad with 128 buttons, 2 hat switches and x, y, z, rZ, rX, rY and 2 sliders (no simulation controls enabled by default)

    // Changing BleControllerConfig after the begin function has no effect, unless you call the begin function again
}

void loop()
{
    if (BleController.isConnected())
    {
        Serial.println("Press buttons 1, 32, 64 and 128. Set hat 1 to down right and hat 2 to up left");

        // Press buttons 5, 32, 64 and 128
        BleController.press(BUTTON_5);
        BleController.press(BUTTON_32);
        BleController.press(BUTTON_64);
        BleController.press(BUTTON_128);

        // Move all axes to max.
        BleController.setLeftThumb(32767, 32767);  // or BleController.setX(32767); and BleController.setY(32767);
        BleController.setRightThumb(32767, 32767); // or BleController.setZ(32767); and BleController.setRZ(32767);
        BleController.setLeftTrigger(32767);       // or BleController.setRX(32767);
        BleController.setRightTrigger(32767);      // or BleController.setRY(32767);
        BleController.setSlider1(32767);
        BleController.setSlider2(32767);

        // Set hat 1 to down right and hat 2 to up left (hats are otherwise centered by default)
        BleController.setHat1(DPAD_DOWN_RIGHT); // or BleController.setHat1(HAT_DOWN_RIGHT);
        BleController.setHat2(DPAD_UP_LEFT);    // or BleController.setHat2(HAT_UP_LEFT);
        // Or BleController.setHats(DPAD_DOWN_RIGHT, DPAD_UP_LEFT);

        // Send the gamepad report
        BleController.sendReport();
        delay(500);

        Serial.println("Release button 5 and 64. Move all axes to min. Set hat 1 and 2 to centred.");
        BleController.release(BUTTON_5);
        BleController.release(BUTTON_64);
        BleController.setAxes(0, 0, 0, 0, 0, 0, 0, 0);
        BleController.setHats(DPAD_CENTERED, HAT_CENTERED);
        BleController.sendReport();
        delay(500);
    }
}

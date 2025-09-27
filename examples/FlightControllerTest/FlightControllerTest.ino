/*
 * Flight controller test
 */

#include <Arduino.h>
#include <BleController.h>

#define numOfButtons 16
#define numOfHatSwitches 0
#define enableX true
#define enableY true
#define enableZ false
#define enableRX false
#define enableRY false
#define enableRZ false
#define enableSlider1 false
#define enableSlider2 false
#define enableRudder true
#define enableThrottle true
#define enableAccelerator false
#define enableBrake true
#define enableSteering false

BleController BleController("BLE Flight Controller", "lemmingDev", 100);

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // Setup controller with 16 buttons (plus start and select), accelerator, brake and steering
    BleControllerConfiguration BleControllerConfig;
    BleControllerConfig.setAutoReport(false);
    BleControllerConfig.setControllerType(CONTROLLER_TYPE_MULTI_AXIS); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
    BleControllerConfig.setButtonCount(numOfButtons);
    BleControllerConfig.setIncludeStart(true);
    BleControllerConfig.setIncludeSelect(true);
    BleControllerConfig.setWhichAxes(enableX, enableY, enableZ, enableRX, enableRY, enableRZ, enableSlider1, enableSlider2);      // Can also be done per-axis individually. All are true by default
    BleControllerConfig.setWhichSimulationControls(enableRudder, enableThrottle, enableAccelerator, enableBrake, enableSteering); // Can also be done per-control individually. All are false by default
    BleControllerConfig.setHatSwitchCount(numOfHatSwitches);                                                                      // 1 by default
    // Some non-Windows operating systems and web based gamepad testers don't like min axis set below 0, so 0 is set by default
    BleControllerConfig.setAxesMin(0x8001); // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
    BleControllerConfig.setAxesMax(0x7FFF); // 32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal 
    // Shows how simulation control min/max axes can be set independently of the other axes
    BleControllerConfig.setSimulationMin(-255); // -255 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
    BleControllerConfig.setSimulationMax(255); // 255 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
    BleController.begin(&BleControllerConfig);

    // changing BleControllerConfig after the begin function has no effect, unless you call the begin function again

    // Set throttle and rudder to min
    BleController.setThrottle(-255);
    BleController.setRudder(-255);

    // Set x and y axes to center
    BleController.setX(0);
    BleController.setY(0);
}

void loop()
{
    if (BleController.isConnected())
    {
        Serial.println("Press all buttons one by one");
        for (int i = 1; i <= numOfButtons; i += 1)
        {
            BleController.press(i);
            BleController.sendReport();
            delay(100);
            BleController.release(i);
            BleController.sendReport();
            delay(25);
        }

        Serial.println("Press start and select");
        BleController.pressSelect();
        BleController.sendReport();
        delay(100);
        BleController.releaseSelect();
        BleController.sendReport();
        delay(100);
        
        BleController.pressStart();
        BleController.sendReport();
        delay(100);
        BleController.releaseStart();
        BleController.sendReport();
        delay(100);
        
        Serial.println("Move x axis from center to max");
        for (int i = 0; i > -32767; i -= 256)
        {
            BleController.setX(i);
            BleController.sendReport();
            delay(10);
        }

        Serial.println("Move x axis from min to max");
        for (int i = -32767; i < 32767; i += 256)
        {
            BleController.setX(i);
            BleController.sendReport();
            delay(10);
        }

        Serial.println("Move x axis from max to center");
        for (int i = 32767; i > 0; i -= 256)
        {
            BleController.setX(i);
            BleController.sendReport();
            delay(10);
        }
        BleController.setX(0);
        BleController.sendReport();

        Serial.println("Move y axis from center to max");
        for (int i = 0; i > -32767; i -= 256)
        {
            BleController.setY(i);
            BleController.sendReport();
            delay(10);
        }

        Serial.println("Move y axis from min to max");
        for (int i = -32767; i < 32767; i += 256)
        {
            BleController.setY(i);
            BleController.sendReport();
            delay(10);
        }

        Serial.println("Move y axis from max to center");
        for (int i = 32767; i > 0; i -= 256)
        {
            BleController.setY(i);
            BleController.sendReport();
            delay(10);
        }
        BleController.setY(0);
        BleController.sendReport();

        Serial.println("Move rudder from min to max");
        // for(int i = 255 ; i > -255 ; i -= 2)    //Use this for loop setup instead if rudder is reversed
        for (int i = -255; i < 255; i += 2)
        {
            BleController.setRudder(i);
            BleController.sendReport();
            delay(10);
        }
        BleController.setRudder(0);
        BleController.sendReport();

        Serial.println("Move throttle from min to max");
        for (int i = -255; i < 255; i += 2)
        {
            BleController.setThrottle(i);
            BleController.sendReport();
            delay(10);
        }
        BleController.setThrottle(-255);
        BleController.sendReport();

        Serial.println("Move brake from min to max");
        for (int i = -255; i < 255; i += 2)
        {
            BleController.setBrake(i);
            BleController.sendReport();
            delay(10);
        }
        BleController.setBrake(-255);
        BleController.sendReport();
    }
}
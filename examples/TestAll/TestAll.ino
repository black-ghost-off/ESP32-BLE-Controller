/*
 * Test all Controller buttons, axes and dpad
 */

#include <Arduino.h>
#include <BleController.h>

#define numOfButtons 64
#define numOfHatSwitches 4

BleController BleController;
BleControllerConfiguration BleControllerConfig;

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting BLE work!");
    BleControllerConfig.setAutoReport(false);
    BleControllerConfig.setControllerType(CONTROLLER_TYPE_CONTROLLER); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_CONTROLLER (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
    BleControllerConfig.setButtonCount(numOfButtons);
    BleControllerConfig.setHatSwitchCount(numOfHatSwitches);
    BleControllerConfig.setVid(0xe502);
    BleControllerConfig.setPid(0xabcd);
    // Some non-Windows operating systems and web based Controller testers don't like min axis set below 0, so 0 is set by default
    //BleControllerConfig.setAxesMin(0x8001); // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
    BleControllerConfig.setAxesMin(0x0000); // 0 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
    BleControllerConfig.setAxesMax(0x7FFF); // 32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal 
    BleController.begin(&BleControllerConfig); // Simulation controls, special buttons and hats 2/3/4 are disabled by default

    // Changing BleControllerConfig after the begin function has no effect, unless you call the begin function again
}

void loop()
{
    if (BleController.isConnected())
    {
        Serial.println("\nn--- Axes Decimal ---");
        Serial.print("Axes Min: ");
        Serial.println(BleControllerConfig.getAxesMin());
        Serial.print("Axes Max: ");
        Serial.println(BleControllerConfig.getAxesMax());
        Serial.println("\nn--- Axes Hex ---");
        Serial.print("Axes Min: ");
        Serial.println(BleControllerConfig.getAxesMin(), HEX);
        Serial.print("Axes Max: ");
        Serial.println(BleControllerConfig.getAxesMax(), HEX);        
        
        Serial.println("\n\nPress all buttons one by one");
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
        BleController.pressStart();
        BleController.sendReport();
        delay(100);
        BleController.pressSelect();
        BleController.sendReport();
        delay(100);
        BleController.releaseStart();
        BleController.sendReport();
        delay(100);
        BleController.releaseSelect();
        BleController.sendReport();

        Serial.println("Move all axis simultaneously from min to max");
        for (int i = BleControllerConfig.getAxesMin(); i < BleControllerConfig.getAxesMax(); i += (BleControllerConfig.getAxesMax() / 256) + 1)
        {
            BleController.setAxes(i, i, i, i, i, i);       // (x, y, z, rx, ry, rz)
            //BleController.setHIDAxes(i, i, i, i, i, i);  // (x, y, z, rz, rx, ry)
            BleController.sendReport();
            delay(10);
        }
        BleController.setAxes(); // Reset all axes to zero
        BleController.sendReport();

        Serial.println("Move all sliders simultaneously from min to max");
        for (int i = BleControllerConfig.getAxesMin(); i < BleControllerConfig.getAxesMax(); i += (BleControllerConfig.getAxesMax() / 256) + 1)
        {
            BleController.setSliders(i, i);
            BleController.sendReport();
            delay(10);
        }
        BleController.setSliders(); // Reset all sliders to zero
        BleController.sendReport();

        Serial.println("Send hat switch 1 one by one in an anticlockwise rotation");
        for (int i = 8; i >= 0; i--)
        {
            BleController.setHat1(i);
            BleController.sendReport();
            delay(200);
        }

        Serial.println("Send hat switch 2 one by one in an anticlockwise rotation");
        for (int i = 8; i >= 0; i--)
        {
            BleController.setHat2(i);
            BleController.sendReport();
            delay(200);
        }

        Serial.println("Send hat switch 3 one by one in an anticlockwise rotation");
        for (int i = 8; i >= 0; i--)
        {
            BleController.setHat3(i);
            BleController.sendReport();
            delay(200);
        }

        Serial.println("Send hat switch 4 one by one in an anticlockwise rotation");
        for (int i = 8; i >= 0; i--)
        {
            BleController.setHat4(i);
            BleController.sendReport();
            delay(200);
        }

        //    Simulation controls are disabled by default
        //    Serial.println("Move all simulation controls simultaneously from min to max");
        //    for (int i = BleControllerConfig.getSimulationMin(); i < BleControllerConfig.getSimulationMax(); i += (BleControllerConfig.getAxesMax() / 256) + 1)
        //    {
        //      BleController.setRudder(i);
        //      BleController.setThrottle(i);
        //      BleController.setAccelerator(i);
        //      BleController.setBrake(i);
        //      BleController.setSteering(i);
        //      BleController.sendReport();
        //      delay(10);
        //    }
        //    BleController.setSimulationControls(); //Reset all simulation controls to zero
        BleController.sendReport();
    }
}

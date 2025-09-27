/*
 * This code programs a number of pins on an ESP32 as buttons on a BLE Controller
 *
 * It uses arrays to cut down on code
 *
 * Before using, adjust the numOfButtons, buttonPins and physicalButtons to suit your senario
 *
 */

#include <Arduino.h>
#include <BleController.h> // https://github.com/lemmingDev/ESP32-BLE-Controller

BleController BleController;

#define numOfButtons 10

byte previousButtonStates[numOfButtons];
byte currentButtonStates[numOfButtons];
byte buttonPins[numOfButtons] = {0, 35, 17, 18, 19, 23, 25, 26, 27, 32};
byte physicalButtons[numOfButtons] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

void setup()
{
    for (byte currentPinIndex = 0; currentPinIndex < numOfButtons; currentPinIndex++)
    {
        pinMode(buttonPins[currentPinIndex], INPUT_PULLUP);
        previousButtonStates[currentPinIndex] = HIGH;
        currentButtonStates[currentPinIndex] = HIGH;
    }

    BleControllerConfiguration BleControllerConfig;
    BleControllerConfig.setAutoReport(false);
    BleControllerConfig.setButtonCount(numOfButtons);
    BleController.begin(&BleControllerConfig);

    // changing BleControllerConfig after the begin function has no effect, unless you call the begin function again
}

void loop()
{
    if (BleController.isConnected())
    {
        for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++)
        {
            currentButtonStates[currentIndex] = digitalRead(buttonPins[currentIndex]);

            if (currentButtonStates[currentIndex] != previousButtonStates[currentIndex])
            {
                if (currentButtonStates[currentIndex] == LOW)
                {
                    BleController.press(physicalButtons[currentIndex]);
                }
                else
                {
                    BleController.release(physicalButtons[currentIndex]);
                }
            }
        }

        if (memcmp((const void *)currentButtonStates, (const void *)previousButtonStates, sizeof(currentButtonStates)) != 0)
        {
            for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++)
            {
                previousButtonStates[currentIndex] = currentButtonStates[currentIndex];
            }

            BleController.sendReport();
        }

        delay(20);
    }
}
/*
 * A simple sketch that maps a single pin on the ESP32 to a single button on the controller
 */

#include <Arduino.h>
#include <BleController.h> // https://github.com/lemmingDev/ESP32-BLE-Gamepad

#define BUTTONPIN 35 // Pin button is attached to

BleController BleController;

int previousButton1State = HIGH;

void setup()
{
    pinMode(BUTTONPIN, INPUT_PULLUP);
    BleController.begin();
}

void loop()
{
    if (BleController.isConnected())
    {

        int currentButton1State = digitalRead(BUTTONPIN);

        if (currentButton1State != previousButton1State)
        {
            if (currentButton1State == LOW)
            {
                BleController.press(BUTTON_1);
            }
            else
            {
                BleController.release(BUTTON_1);
            }
        }
        previousButton1State = currentButton1State;
    }
}
#include <Arduino.h>
#include <Bounce2.h>    // https://github.com/thomasfredericks/Bounce2
#include <BleController.h> // https://github.com/black-ghost-off/ESP32-BLE-Controller

#define BOUNCE_WITH_PROMPT_DETECTION // Make button state changes available immediately
#define BUTTON_PIN 35
#define LED_PIN 13

Bounce debouncer = Bounce(); // Instantiate a Bounce object
BleController BleController;       // Instantiate a BleController object

void setup()
{
    BleController.begin(); // Begin the Controller

    pinMode(BUTTON_PIN, INPUT_PULLUP); // Setup the button with an internal pull-up

    debouncer.attach(BUTTON_PIN); // After setting up the button, setup the Bounce instance :
    debouncer.interval(5);        // interval in ms

    pinMode(LED_PIN, OUTPUT); // Setup the LED :
}

void loop()
{
    if (BleController.isConnected())
    {
        debouncer.update(); // Update the Bounce instance

        int value = debouncer.read(); // Get the updated value

        // Press/release Controller button and turn on or off the LED as determined by the state
        if (value == LOW)
        {
            digitalWrite(LED_PIN, HIGH);
            BleController.press(BUTTON_1);
        }
        else
        {
            digitalWrite(LED_PIN, LOW);
            BleController.release(BUTTON_1);
        }
    }
}

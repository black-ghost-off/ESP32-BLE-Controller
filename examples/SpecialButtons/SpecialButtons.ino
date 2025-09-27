#include <Arduino.h>
#include <BleController.h>

BleController BleController;

void setup()
{
    Serial.begin(115200);
    BleControllerConfiguration BleControllerConfig;
    BleControllerConfig.setWhichSpecialButtons(true, true, true, true, true, true, true, true);
    // Can also enable special buttons individually with the following <-- They are all disabled by default
    // BleControllerConfig.setIncludeStart(true);
    // BleControllerConfig.setIncludeSelect(true);
    // BleControllerConfig.setIncludeMenu(true);
    // BleControllerConfig.setIncludeHome(true);
    // BleControllerConfig.setIncludeBack(true);
    // BleControllerConfig.setIncludeVolumeInc(true);
    // BleControllerConfig.setIncludeVolumeDec(true);
    // BleControllerConfig.setIncludeVolumeMute(true);
    BleController.begin(&BleControllerConfig);

    // Changing BleControllerConfig after the begin function has no effect, unless you call the begin function again
}

void loop()
{
    if (BleController.isConnected())
    {
        Serial.println("Pressing start and select");
        BleController.pressStart();
        delay(100);
        BleController.releaseStart();
        delay(100);
        BleController.pressSelect();
        delay(100);
        BleController.releaseSelect();
        delay(100);

        Serial.println("Increasing volume");
        BleController.pressVolumeInc();
        delay(100);
        BleController.releaseVolumeInc();
        delay(100);
        BleController.pressVolumeInc();
        delay(100);
        BleController.releaseVolumeInc();
        delay(100);
        
        Serial.println("Muting volume");
        BleController.pressVolumeMute();
        delay(100);
        BleController.releaseVolumeMute();
        delay(1000);
        BleController.pressVolumeMute();
        delay(100);
        BleController.releaseVolumeMute();


        Serial.println("Pressing menu and back");
        BleController.pressMenu();
        delay(100);
        BleController.releaseMenu();
        delay(100);
        BleController.pressBack();
        delay(100);
        BleController.releaseBack();
        delay(100);

        Serial.println("Pressing home");
        BleController.pressHome();
        delay(100);
        BleController.releaseHome();
        delay(2000);
    }
}

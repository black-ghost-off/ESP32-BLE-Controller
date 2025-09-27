/*
   Sets BLE characteristic options
   Use BLE Scanner etc on Android to see them

   Also shows how to set transmit power during initial configuration,
   or at any stage whilst running by using BleController.setTXPowerLevel(int8_t)

   The only valid values are: -12, -9, -6, -3, 0, 3, 6 and 9
   Values correlate to dbm

   You can get the currently set TX power level by calling BleController.setTXPowerLevel()

*/

#include <Arduino.h>
#include <BleController.h>

int8_t txPowerLevel = 3;

BleController BleController("Custom Contoller Name", "lemmingDev", 100); // Set custom device name, manufacturer and initial battery level
BleControllerConfiguration BleControllerConfig;                          // Create a BleControllerConfiguration object to store all of the options

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  BleControllerConfig.setAutoReport(false);
  BleControllerConfig.setControllerType(CONTROLLER_TYPE_CONTROLLER); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_CONTROLLER (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
  BleControllerConfig.setVid(0xe502);
  BleControllerConfig.setPid(0xabcd);
  BleControllerConfig.setTXPowerLevel(txPowerLevel);  // Defaults to 9 if not set. (Range: -12 to 9 dBm)

  BleControllerConfig.setModelNumber("1.0");
  BleControllerConfig.setSoftwareRevision("Software Rev 1");
  BleControllerConfig.setSerialNumber("9876543210");
  BleControllerConfig.setFirmwareRevision("2.0");
  BleControllerConfig.setHardwareRevision("1.7");

  // Some non-Windows operating systems and web based Controller testers don't like min axis set below 0, so 0 is set by default
  //BleControllerConfig.setAxesMin(0x8001); // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
  BleControllerConfig.setAxesMin(0x0000); // 0 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
  BleControllerConfig.setAxesMax(0x7FFF); // 32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal

  BleController.begin(&BleControllerConfig); // Begin Controller with configuration options
  
  // Change power level to 6
  BleController.setTXPowerLevel(6);    // The default of 9 (strongest transmit power level) will be used if not set

}

void loop()
{
  if (BleController.isConnected())
  {

  }
}

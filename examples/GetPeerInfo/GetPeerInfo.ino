/*
   A simple sketch that, upon a button press, shows peer info

   BleController.getPeerInfo(); returns type NimBLEConnInfo
   From there, you have access to all info here https://h2zero.github.io/NimBLE-Arduino/class_nim_b_l_e_conn_info.html

   If you just need the MAC address, you can instead call BleController.getAddress() which returns type NimBLEAddress
   or BleController.getStringAddress() to get it directly as a string

   This sketch also shows how to access the information used to configure the BLE device such as vid, pid, model number and software revision etc
   See CharacteristicsConfiguration.ino example to see how to set them
*/

#include <Arduino.h>
#include <BleController.h> // https://github.com/lemmingDev/ESP32-BLE-Gamepad

#define PEER_INFO_PIN 0 // Pin button is attached to

BleController BleController;

void setup()
{
  Serial.begin(115200);
  pinMode(PEER_INFO_PIN, INPUT_PULLUP);
  BleController.begin();
}

void loop()
{
  if (BleController.isConnected())
  {

    if (digitalRead(PEER_INFO_PIN) == LOW) // On my board, pin 0 is LOW for pressed
    {
      Serial.println("\n----- OUPTPUT PEER INFORMATION -----\n");

      // Get the HEX address as a string;
      Serial.println(BleController.getStringAddress());

      // Get the HEX address as an NimBLEAddress instance
      NimBLEAddress bleAddress = BleController.getAddress();
      Serial.println(bleAddress.toString().c_str());

      // Get values directly from an NimBLEConnInfo instance
      NimBLEConnInfo peerInfo = BleController.getPeerInfo();

      Serial.println(peerInfo.getAddress().toString().c_str());      // NimBLEAddress
      Serial.println(peerInfo.getIdAddress().toString().c_str());    // NimBLEAddress
      Serial.println(peerInfo.getConnHandle());                      // uint16_t
      Serial.println(peerInfo.getConnInterval());                    // uint16_t
      Serial.println(peerInfo.getConnTimeout());                     // uint16_t
      Serial.println(peerInfo.getConnLatency());                     // uint16_t
      Serial.println(peerInfo.getMTU());                             // uint16_t
      Serial.println(peerInfo.isMaster());                           // bool
      Serial.println(peerInfo.isSlave());                            // bool
      Serial.println(peerInfo.isBonded());                           // bool
      Serial.println(peerInfo.isEncrypted());                        // bool
      Serial.println(peerInfo.isAuthenticated());                    // bool
      Serial.println(peerInfo.getSecKeySize());                      // uint8_t

      Serial.println("\n----- OUPTPUT CONFIGURATION INFORMATION -----\n");
      Serial.println(BleController.getDeviceName());
      Serial.println(BleController.getDeviceManufacturer());
      Serial.println(BleController.configuration.getModelNumber());
      Serial.println(BleController.configuration.getSoftwareRevision());
      Serial.println(BleController.configuration.getSerialNumber());
      Serial.println(BleController.configuration.getFirmwareRevision());
      Serial.println(BleController.configuration.getHardwareRevision());
      Serial.println(BleController.configuration.getVid(), HEX);
      Serial.println(BleController.configuration.getPid(), HEX);
      Serial.println(BleController.configuration.getGuidVersion());
      Serial.println(BleController.configuration.getTXPowerLevel());
      Serial.println();
      delay(1000);
    }
  }
  else
  {
    Serial.println("No device connected");
    delay(1000);
  }
}
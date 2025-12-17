#include "BleController.h"
#include "BleConnectionStatus.h"
#include "BleControllerConfiguration.h"
#include "BleKeyboardKeys.h"
#include "HIDTypes.h"
#include "NimBLEHIDDevice.h"
#include "NimBLELog.h"
#include "sdkconfig.h"
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>

#include <stdexcept>

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG "BleController"
#else
#include "esp_log.h"
static const char *LOG_TAG = "BleController";
#endif

#define SERVICE_UUID_DEVICE_INFORMATION "180A" // Service - Device information

#define CHARACTERISTIC_UUID_MODEL_NUMBER                                       \
  "2A24" // Characteristic - Model Number String - 0x2A24
#define CHARACTERISTIC_UUID_SOFTWARE_REVISION                                  \
  "2A28" // Characteristic - Software Revision String - 0x2A28
#define CHARACTERISTIC_UUID_SERIAL_NUMBER                                      \
  "2A25" // Characteristic - Serial Number String - 0x2A25
#define CHARACTERISTIC_UUID_FIRMWARE_REVISION                                  \
  "2A26" // Characteristic - Firmware Revision String - 0x2A26
#define CHARACTERISTIC_UUID_HARDWARE_REVISION                                  \
  "2A27" // Characteristic - Hardware Revision String - 0x2A27
#define CHARACTERISTIC_UUID_BATTERY_POWER_STATE                                \
  "2A1A" // Characteristic - Battery Power State - 0x2A1A

#define POWER_STATE_UNKNOWN 0         // 0b00
#define POWER_STATE_NOT_SUPPORTED 1   // 0b01
#define POWER_STATE_NOT_PRESENT 2     // 0b10
#define POWER_STATE_NOT_DISCHARGING 2 // 0b10
#define POWER_STATE_NOT_CHARGING 2    // 0b10
#define POWER_STATE_GOOD 2            // 0b10
#define POWER_STATE_PRESENT 3         // 0b11
#define POWER_STATE_DISCHARGING 3     // 0b11
#define POWER_STATE_CHARGING 3        // 0b11
#define POWER_STATE_CRITICAL 3        // 0b11

// BLE Appearance constants
#define HID_CONTROLLER                                                         \
  0x03C0 // Bluetooth SIG Appearance value for HID Controller/Gamepad

#if BLE_CONTROLLER_DEBUG == 1
static void dumpHIDReport(const uint8_t *report, size_t len);
#endif

BleController::BleController(std::string deviceName,
                             std::string deviceManufacturer,
                             uint8_t batteryLevel, bool delayAdvertising)
    : _buttons(), _specialButtons(0), _x(0), _y(0), _z(0), _rX(0), _rY(0),
      _rZ(0), _slider1(0), _slider2(0), _rudder(0), _throttle(0),
      _accelerator(0), _brake(0), _steering(0), _hat1(0), _hat2(0), _hat3(0),
      _hat4(0), _gX(0), _gY(0), _gZ(0), _aX(0), _aY(0), _aZ(0),
      _batteryPowerInformation(0), _dischargingState(0), _chargingState(0),
      _powerLevel(0), hid(0), pCharacteristic_Power_State(0), configuration(),
      pServer(nullptr), nus(nullptr) {
  this->resetButtons();
  this->deviceName = deviceName;
  this->deviceManufacturer = deviceManufacturer;
  this->batteryLevel = batteryLevel;
  this->delayAdvertising = delayAdvertising;
  this->connectionStatus = new BleConnectionStatus();

  // Initialize keyboard and mouse reports
  memset(&_keyboardReport, 0, sizeof(_keyboardReport));
  memset(&_mouseReport, 0, sizeof(_mouseReport));

  hidReportDescriptorSize = 0;
  hidReportSize = 0;
  numOfButtonBytes = 0;
  enableOutputReport = false;
  outputReportLength = 64;
  nusInitialized = false;
}

void BleController::resetButtons() { memset(&_buttons, 0, sizeof(_buttons)); }

void BleController::begin(BleControllerConfiguration *config) {
  configuration =
      *config; // we make a copy, so the user can't change actual values midway
               // through operation, without calling the begin function again

  enableOutputReport = configuration.getEnableOutputReport();
  outputReportLength = configuration.getOutputReportLength();

  uint8_t buttonPaddingBits = 8 - (configuration.getButtonCount() % 8);
  if (buttonPaddingBits == 8) {
    buttonPaddingBits = 0;
  }
  uint8_t specialButtonPaddingBits =
      8 - (configuration.getTotalSpecialButtonCount() % 8);
  if (specialButtonPaddingBits == 8) {
    specialButtonPaddingBits = 0;
  }
  uint8_t numOfAxisBytes = configuration.getAxisCount() * 2;
  uint8_t numOfSimulationBytes = configuration.getSimulationCount() * 2;

  numOfButtonBytes = configuration.getButtonCount() / 8;
  if (buttonPaddingBits > 0) {
    numOfButtonBytes++;
  }

  uint8_t numOfSpecialButtonBytes =
      configuration.getTotalSpecialButtonCount() / 8;
  if (specialButtonPaddingBits > 0) {
    numOfSpecialButtonBytes++;
  }

  uint8_t numOfMotionBytes = 0;
  if (configuration.getIncludeAccelerometer()) {
    numOfMotionBytes += 6;
  }

  if (configuration.getIncludeGyroscope()) {
    numOfMotionBytes += 6;
  }

  hidReportSize = numOfButtonBytes + numOfSpecialButtonBytes + numOfAxisBytes +
                  numOfSimulationBytes + numOfMotionBytes +
                  configuration.getHatSwitchCount();

  // =================== GAMEPAD DESCRIPTOR ===================
  if (configuration.getGamepadEnabled()) {
  // USAGE_PAGE (Generic Desktop)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // USAGE (Joystick - 0x04; Controller - 0x05; Multi-axis Controller - 0x08)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
  tempHidReportDescriptor[hidReportDescriptorSize++] =
      configuration.getControllerType();

  // COLLECTION (Application)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xa1;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // REPORT_ID (Default: 3)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x85;
  tempHidReportDescriptor[hidReportDescriptorSize++] =
      configuration.getHidReportId();

  if (configuration.getButtonCount() > 0) {
    // USAGE_PAGE (Button)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;

    // LOGICAL_MINIMUM (0)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

    // LOGICAL_MAXIMUM (1)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // REPORT_SIZE (1)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // USAGE_MINIMUM (Button 1)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x19;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // USAGE_MAXIMUM (Up to 128 buttons possible)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x29;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        configuration.getButtonCount();

    // REPORT_COUNT (# of buttons)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        configuration.getButtonCount();

    // INPUT (Data,Var,Abs)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

    if (buttonPaddingBits > 0) {

      // REPORT_SIZE (1)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

      // REPORT_COUNT (# of padding bits)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
      tempHidReportDescriptor[hidReportDescriptorSize++] = buttonPaddingBits;

      // INPUT (Const,Var,Abs)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x03;

    } // Padding Bits Needed

  } // Buttons

  if (configuration.getTotalSpecialButtonCount() > 0) {
    // LOGICAL_MINIMUM (0)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

    // LOGICAL_MAXIMUM (1)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // REPORT_SIZE (1)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    if (configuration.getDesktopSpecialButtonCount() > 0) {

      // USAGE_PAGE (Generic Desktop)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

      // REPORT_COUNT
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
      tempHidReportDescriptor[hidReportDescriptorSize++] =
          configuration.getDesktopSpecialButtonCount();

      if (configuration.getIncludeStart()) {
        // USAGE (Start)
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x3D;
      }

      if (configuration.getIncludeSelect()) {
        // USAGE (Select)
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x3E;
      }

      if (configuration.getIncludeMenu()) {
        // USAGE (App Menu)
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x86;
      }

      // INPUT (Data,Var,Abs)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
    }

    if (configuration.getConsumerSpecialButtonCount() > 0) {

      // USAGE_PAGE (Consumer Page)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x0C;

      // REPORT_COUNT
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
      tempHidReportDescriptor[hidReportDescriptorSize++] =
          configuration.getConsumerSpecialButtonCount();

      if (configuration.getIncludeHome()) {
        // USAGE (Home)
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x0A;
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x23;
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
      }

      if (configuration.getIncludeBack()) {
        // USAGE (Back)
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x0A;
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x24;
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
      }

      if (configuration.getIncludeVolumeInc()) {
        // USAGE (Volume Increment)
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0xE9;
      }

      if (configuration.getIncludeVolumeDec()) {
        // USAGE (Volume Decrement)
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0xEA;
      }

      if (configuration.getIncludeVolumeMute()) {
        // USAGE (Mute)
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
        tempHidReportDescriptor[hidReportDescriptorSize++] = 0xE2;
      }

      // INPUT (Data,Var,Abs)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
    }

    if (specialButtonPaddingBits > 0) {

      // REPORT_SIZE (1)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

      // REPORT_COUNT (# of padding bits)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
      tempHidReportDescriptor[hidReportDescriptorSize++] =
          specialButtonPaddingBits;

      // INPUT (Const,Var,Abs)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x03;

    } // Padding Bits Needed

  } // Special Buttons

  if (configuration.getAxisCount() > 0) {
    // USAGE_PAGE (Generic Desktop)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // USAGE (Pointer)
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // LOGICAL_MINIMUM (-32767)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x16;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        lowByte(configuration.getAxesMin());
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        highByte(configuration.getAxesMin());
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;    // Use
    // these two lines for 0 min
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;    // Use
    // these two lines for -32767 min
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x80;

    // LOGICAL_MAXIMUM (+32767)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x26;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        lowByte(configuration.getAxesMax());
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        highByte(configuration.getAxesMax());
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0xFF;	// Use
    // these two lines for 255 max
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0xFF;	// Use
    // these two lines for +32767 max
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x7F;

    // REPORT_SIZE (16)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x10;

    // REPORT_COUNT (configuration.getAxisCount())
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        configuration.getAxisCount();

    // COLLECTION (Physical)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xA1;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

    if (configuration.getIncludeXAxis()) {
      // USAGE (X)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x30;
    }

    if (configuration.getIncludeYAxis()) {
      // USAGE (Y)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x31;
    }

    if (configuration.getIncludeZAxis()) {
      // USAGE (Z)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x32;
    }

    if (configuration.getIncludeRzAxis()) {
      // USAGE (Rz)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x35;
    }

    if (configuration.getIncludeRxAxis()) {
      // USAGE (Rx)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x33;
    }

    if (configuration.getIncludeRyAxis()) {
      // USAGE (Ry)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x34;
    }

    if (configuration.getIncludeSlider1()) {
      // USAGE (Slider)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x36;
    }

    if (configuration.getIncludeSlider2()) {
      // USAGE (Slider)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x36;
    }

    // INPUT (Data,Var,Abs)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

    // END_COLLECTION (Physical)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;

  } // X, Y, Z, Rx, Ry, and Rz Axis

  if (configuration.getSimulationCount() > 0) {

    // USAGE_PAGE (Simulation Controls)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

    // LOGICAL_MINIMUM (-32767)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x16;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        lowByte(configuration.getSimulationMin());
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        highByte(configuration.getSimulationMin());
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;	    //
    // Use these two lines for 0 min
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;	    //
    // Use these two lines for -32767 min
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x80;

    // LOGICAL_MAXIMUM (+32767)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x26;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        lowByte(configuration.getSimulationMax());
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        highByte(configuration.getSimulationMax());
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0xFF;	    //
    // Use these two lines for 255 max
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0xFF;	    //
    // Use these two lines for +32767 max
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x7F;

    // REPORT_SIZE (16)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x10;

    // REPORT_COUNT (configuration.getSimulationCount())
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        configuration.getSimulationCount();

    // COLLECTION (Physical)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xA1;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

    if (configuration.getIncludeRudder()) {
      // USAGE (Rudder)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0xBA;
    }

    if (configuration.getIncludeThrottle()) {
      // USAGE (Throttle)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0xBB;
    }

    if (configuration.getIncludeAccelerator()) {
      // USAGE (Accelerator)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0xC4;
    }

    if (configuration.getIncludeBrake()) {
      // USAGE (Brake)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0xC5;
    }

    if (configuration.getIncludeSteering()) {
      // USAGE (Steering)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0xC8;
    }

    // INPUT (Data,Var,Abs)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

    // END_COLLECTION (Physical)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;

  } // Simulation Controls

  // Gyroscope
  if (configuration.getIncludeGyroscope()) {
    // COLLECTION (Physical)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xA1;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

    // USAGE_PAGE (Generic Desktop)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // USAGE (Gyroscope - Rotational X - Rx)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x33;

    // USAGE (Rotational - Rotational Y - Ry)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x34;

    // USAGE (Rotational - Rotational Z - Rz)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x35;

    // LOGICAL_MINIMUM (-32767)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x16;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        lowByte(configuration.getMotionMin());
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        highByte(configuration.getMotionMin());
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;	    //
    // Use these two lines for 0 min
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;	    //
    // Use these two lines for -32767 min
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x80;

    // LOGICAL_MAXIMUM (+32767)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x26;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        lowByte(configuration.getMotionMax());
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        highByte(configuration.getMotionMax());
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0xFF;	    //
    // Use these two lines for 255 max
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0xFF;	    //
    // Use these two lines for +32767 max
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x7F;

    // REPORT_SIZE (16)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x10;

    // REPORT_COUNT (3)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x03;

    // INPUT (Data,Var,Abs)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

    // END_COLLECTION (Physical)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;

  } // Gyroscope

  // Accelerometer
  if (configuration.getIncludeAccelerometer()) {
    // COLLECTION (Physical)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xA1;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

    // USAGE_PAGE (Generic Desktop)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // USAGE (Accelerometer - Vector X - Vx)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x40;

    // USAGE (Accelerometer - Vector Y - Vy)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x41;

    // USAGE (Accelerometer - Vector Z - Vz)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x42;

    // LOGICAL_MINIMUM (-32767)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x16;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        lowByte(configuration.getMotionMin());
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        highByte(configuration.getMotionMin());
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;	    //
    // Use these two lines for 0 min
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;	    //
    // Use these two lines for -32767 min
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x80;

    // LOGICAL_MAXIMUM (+32767)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x26;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        lowByte(configuration.getMotionMax());
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        highByte(configuration.getMotionMax());
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0xFF;	    //
    // Use these two lines for 255 max
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0xFF;	    //
    // Use these two lines for +32767 max
    // tempHidReportDescriptor[hidReportDescriptorSize++] = 0x7F;

    // REPORT_SIZE (16)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x10;

    // REPORT_COUNT (3)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x03;

    // INPUT (Data,Var,Abs)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

    // END_COLLECTION (Physical)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;

  } // Accelerometer

  if (configuration.getHatSwitchCount() > 0) {

    // COLLECTION (Physical)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xA1;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

    // USAGE_PAGE (Generic Desktop)
    tempHidReportDescriptor[hidReportDescriptorSize++] = USAGE_PAGE(1);
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // USAGE (Hat Switch)
    for (int currentHatIndex = 0;
         currentHatIndex < configuration.getHatSwitchCount();
         currentHatIndex++) {
      tempHidReportDescriptor[hidReportDescriptorSize++] = USAGE(1);
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x39;
    }

    // Logical Min (1)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // Logical Max (8)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x08;

    // Physical Min (0)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x35;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

    // Physical Max (315)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x46;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x3B;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // Unit (SI Rot : Ang Pos)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x65;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x12;

    // Report Size (8)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x08;

    // Report Count (4)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
    tempHidReportDescriptor[hidReportDescriptorSize++] =
        configuration.getHatSwitchCount();

    // Input (Data, Variable, Absolute)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x42;

    // END_COLLECTION (Physical)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;
  } // Hat Switches

  if (configuration.getEnableOutputReport()) {
    // Usage Page (Vendor Defined 0xFF00)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x06;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xFF;

    // Usage (Vendor Usage 0x01)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // Usage (0x01)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // Logical Minimum (0)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

    // Logical Maximum (255)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x26;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xFF;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

    // Report Size (8 bits)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x08;

    if (configuration.getOutputReportLength() <= 0xFF) {
      // Report Count (0~255 bytes)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
      tempHidReportDescriptor[hidReportDescriptorSize++] =
          configuration.getOutputReportLength();
    } else {
      // Report Count (0~65535 bytes)
      tempHidReportDescriptor[hidReportDescriptorSize++] = 0x96;
      tempHidReportDescriptor[hidReportDescriptorSize++] =
          lowByte(configuration.getOutputReportLength());
      tempHidReportDescriptor[hidReportDescriptorSize++] =
          highByte(configuration.getOutputReportLength());
    }

    // Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
    // Position,Non-volatile)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x91;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
  }

  // END_COLLECTION (Application) - End Controller collection
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;
  } // End Gamepad Enabled

  // =================== KEYBOARD DESCRIPTOR ===================
  if (configuration.getKeyboardEnabled()) {
  // USAGE_PAGE (Generic Desktop)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // USAGE (Keyboard)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x06;

  // COLLECTION (Application)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xa1;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // REPORT_ID (Keyboard)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x85;
  tempHidReportDescriptor[hidReportDescriptorSize++] = KEYBOARD_REPORT_ID;

  // USAGE_PAGE (Keyboard/Keypad)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x07;

  // USAGE_MINIMUM (Keyboard LeftControl)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x19;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xe0;

  // USAGE_MAXIMUM (Keyboard Right GUI)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x29;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xe7;

  // LOGICAL_MINIMUM (0)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

  // LOGICAL_MAXIMUM (1)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // REPORT_SIZE (1)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // REPORT_COUNT (8)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x08;

  // INPUT (Data,Var,Abs)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

  // REPORT_COUNT (1) - Reserved byte
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // REPORT_SIZE (8)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x08;

  // INPUT (Const,Var,Abs) - Reserved byte
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x03;

  // REPORT_COUNT (configurable key count, default 6) - Key array
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
  tempHidReportDescriptor[hidReportDescriptorSize++] = configuration.getKeyboardKeyCount();

  // REPORT_SIZE (8)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x08;

  // LOGICAL_MINIMUM (0)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

  // LOGICAL_MAXIMUM (101)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x65;

  // USAGE_PAGE (Keyboard/Keypad)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x07;

  // USAGE_MINIMUM (Reserved)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x19;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

  // USAGE_MAXIMUM (Keyboard Application)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x29;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x65;

  // INPUT (Data,Array,Abs)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

  // END_COLLECTION (Application) - End keyboard collection
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;
  } // End Keyboard Enabled

  // =================== MOUSE DESCRIPTOR ===================
  if (configuration.getMouseEnabled()) {
  // Based on ESP32-NimBLE-Mouse reference implementation
  // Report format: buttons(1 byte) + X(1) + Y(1) + wheel(1) + hWheel(1) = 5
  // bytes

  // USAGE_PAGE (Generic Desktop)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // USAGE (Mouse)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

  // COLLECTION (Application)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xa1;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // USAGE (Pointer)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // COLLECTION (Physical)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xa1;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

  // REPORT_ID (Mouse)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x85;
  tempHidReportDescriptor[hidReportDescriptorSize++] = MOUSE_REPORT_ID;

  // ---- Buttons (configurable button count + padding bits = 1 byte) ----
  uint8_t mouseButtonCount = configuration.getMouseButtonCount();
  uint8_t mousePaddingBits = 8 - mouseButtonCount;
  
  // USAGE_PAGE (Button)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;

  // USAGE_MINIMUM (Button 1)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x19;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // USAGE_MAXIMUM (Button N - configurable)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x29;
  tempHidReportDescriptor[hidReportDescriptorSize++] = mouseButtonCount;

  // LOGICAL_MINIMUM (0)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

  // LOGICAL_MAXIMUM (1)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // REPORT_SIZE (1)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // REPORT_COUNT (N button bits - configurable)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
  tempHidReportDescriptor[hidReportDescriptorSize++] = mouseButtonCount;

  // INPUT (Data,Var,Abs) - button bits
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

  // REPORT_SIZE (padding bits)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
  tempHidReportDescriptor[hidReportDescriptorSize++] = mousePaddingBits;

  // REPORT_COUNT (1)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // INPUT (Const,Var,Abs) - Padding
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x03;

  // ---- X/Y (2 bytes) ----
  // USAGE_PAGE (Generic Desktop)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // USAGE (X)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x30;

  // USAGE (Y)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x31;

  // LOGICAL_MINIMUM (-127)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;

  // LOGICAL_MAXIMUM (127)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x7f;

  // REPORT_SIZE (8)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x08;

  // REPORT_COUNT (2) - X and Y only
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

  // INPUT (Data,Var,Rel) - Relative movement
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x06;

  // ---- Vertical Wheel (1 byte) - Conditional ----
  if (configuration.getMouseWheelEnabled()) {
  // USAGE (Wheel)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x38;

  // LOGICAL_MINIMUM (-127)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;

  // LOGICAL_MAXIMUM (127)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x7f;

  // REPORT_SIZE (8)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x08;

  // REPORT_COUNT (1) - Wheel only
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // INPUT (Data,Var,Rel) - Relative movement
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x06;
  } // End Mouse Wheel Enabled

  // ---- Horizontal Wheel (1 byte) - Conditional ----
  if (configuration.getMouseHWheelEnabled()) {
  // USAGE_PAGE (Consumer Devices)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x0c;

  // USAGE (AC Pan)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x0a;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x38;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

  // LOGICAL_MINIMUM (-127)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;

  // LOGICAL_MAXIMUM (127)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x7f;

  // REPORT_SIZE (8)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x08;

  // REPORT_COUNT (1)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

  // INPUT (Data,Var,Rel)
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0x06;
  } // End Mouse Horizontal Wheel Enabled

  // END_COLLECTION (Physical) - End mouse pointer collection
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;

  // END_COLLECTION (Application) - End mouse collection
  tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;
  } // End Mouse Enabled

  // Set task priority from 5 to 1 in order to get ESP32-C3 working
  xTaskCreate(this->taskServer, "server", 20000, (void *)this, 1, NULL);
}

void BleController::end(void) {}

void BleController::setAxes(int16_t x, int16_t y, int16_t z, int16_t rX,
                            int16_t rY, int16_t rZ, int16_t slider1,
                            int16_t slider2) {
  if (x == -32768) {
    x = -32767;
  }
  if (y == -32768) {
    y = -32767;
  }
  if (z == -32768) {
    z = -32767;
  }
  if (rZ == -32768) {
    rZ = -32767;
  }
  if (rX == -32768) {
    rX = -32767;
  }
  if (rY == -32768) {
    rY = -32767;
  }
  if (slider1 == -32768) {
    slider1 = -32767;
  }
  if (slider2 == -32768) {
    slider2 = -32767;
  }

  _x = x;
  _y = y;
  _z = z;
  _rZ = rZ;
  _rX = rX;
  _rY = rY;
  _slider1 = slider1;
  _slider2 = slider2;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setHIDAxes(int16_t x, int16_t y, int16_t z, int16_t rZ,
                               int16_t rX, int16_t rY, int16_t slider1,
                               int16_t slider2) {
  if (x == -32768) {
    x = -32767;
  }
  if (y == -32768) {
    y = -32767;
  }
  if (z == -32768) {
    z = -32767;
  }
  if (rZ == -32768) {
    rZ = -32767;
  }
  if (rX == -32768) {
    rX = -32767;
  }
  if (rY == -32768) {
    rY = -32767;
  }
  if (slider1 == -32768) {
    slider1 = -32767;
  }
  if (slider2 == -32768) {
    slider2 = -32767;
  }

  _x = x;
  _y = y;
  _z = z;
  _rZ = rZ;
  _rX = rX;
  _rY = rY;
  _slider1 = slider1;
  _slider2 = slider2;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setSimulationControls(int16_t rudder, int16_t throttle,
                                          int16_t accelerator, int16_t brake,
                                          int16_t steering) {
  if (rudder == -32768) {
    rudder = -32767;
  }
  if (throttle == -32768) {
    throttle = -32767;
  }
  if (accelerator == -32768) {
    accelerator = -32767;
  }
  if (brake == -32768) {
    brake = -32767;
  }
  if (steering == -32768) {
    steering = -32767;
  }

  _rudder = rudder;
  _throttle = throttle;
  _accelerator = accelerator;
  _brake = brake;
  _steering = steering;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setHats(signed char hat1, signed char hat2,
                            signed char hat3, signed char hat4) {
  _hat1 = hat1;
  _hat2 = hat2;
  _hat3 = hat3;
  _hat4 = hat4;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setSliders(int16_t slider1, int16_t slider2) {
  if (slider1 == -32768) {
    slider1 = -32767;
  }
  if (slider2 == -32768) {
    slider2 = -32767;
  }

  _slider1 = slider1;
  _slider2 = slider2;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::sendReport(void) {
  if (this->isConnected()) {
    uint8_t currentReportIndex = 0;

    uint8_t m[hidReportSize];

    memset(&m, 0, sizeof(m));
    memcpy(&m, &_buttons, sizeof(_buttons));

    currentReportIndex += numOfButtonBytes;

    if (configuration.getTotalSpecialButtonCount() > 0) {
      m[currentReportIndex++] = _specialButtons;
    }

    if (configuration.getIncludeXAxis()) {
      m[currentReportIndex++] = _x;
      m[currentReportIndex++] = (_x >> 8);
    }
    if (configuration.getIncludeYAxis()) {
      m[currentReportIndex++] = _y;
      m[currentReportIndex++] = (_y >> 8);
    }
    if (configuration.getIncludeZAxis()) {
      m[currentReportIndex++] = _z;
      m[currentReportIndex++] = (_z >> 8);
    }
    if (configuration.getIncludeRzAxis()) {
      m[currentReportIndex++] = _rZ;
      m[currentReportIndex++] = (_rZ >> 8);
    }
    if (configuration.getIncludeRxAxis()) {
      m[currentReportIndex++] = _rX;
      m[currentReportIndex++] = (_rX >> 8);
    }
    if (configuration.getIncludeRyAxis()) {
      m[currentReportIndex++] = _rY;
      m[currentReportIndex++] = (_rY >> 8);
    }

    if (configuration.getIncludeSlider1()) {
      m[currentReportIndex++] = _slider1;
      m[currentReportIndex++] = (_slider1 >> 8);
    }
    if (configuration.getIncludeSlider2()) {
      m[currentReportIndex++] = _slider2;
      m[currentReportIndex++] = (_slider2 >> 8);
    }

    if (configuration.getIncludeRudder()) {
      m[currentReportIndex++] = _rudder;
      m[currentReportIndex++] = (_rudder >> 8);
    }
    if (configuration.getIncludeThrottle()) {
      m[currentReportIndex++] = _throttle;
      m[currentReportIndex++] = (_throttle >> 8);
    }
    if (configuration.getIncludeAccelerator()) {
      m[currentReportIndex++] = _accelerator;
      m[currentReportIndex++] = (_accelerator >> 8);
    }
    if (configuration.getIncludeBrake()) {
      m[currentReportIndex++] = _brake;
      m[currentReportIndex++] = (_brake >> 8);
    }
    if (configuration.getIncludeSteering()) {
      m[currentReportIndex++] = _steering;
      m[currentReportIndex++] = (_steering >> 8);
    }

    if (configuration.getIncludeGyroscope()) {
      m[currentReportIndex++] = _gX;
      m[currentReportIndex++] = (_gX >> 8);
      m[currentReportIndex++] = _gY;
      m[currentReportIndex++] = (_gY >> 8);
      m[currentReportIndex++] = _gZ;
      m[currentReportIndex++] = (_gZ >> 8);
    }

    if (configuration.getIncludeAccelerometer()) {
      m[currentReportIndex++] = _aX;
      m[currentReportIndex++] = (_aX >> 8);
      m[currentReportIndex++] = _aY;
      m[currentReportIndex++] = (_aY >> 8);
      m[currentReportIndex++] = _aZ;
      m[currentReportIndex++] = (_aZ >> 8);
    }

    if (configuration.getHatSwitchCount() > 0) {
      signed char hats[4];

      hats[0] = _hat1;
      hats[1] = _hat2;
      hats[2] = _hat3;
      hats[3] = _hat4;

      for (int currentHatIndex = configuration.getHatSwitchCount() - 1;
           currentHatIndex >= 0; currentHatIndex--) {
        m[currentReportIndex++] = hats[currentHatIndex];
      }
    }

#if BLE_CONTROLLER_DEBUG == 1
    dumpHIDReport(m, sizeof(m));
#endif

    this->inputController->setValue(m, sizeof(m));
    this->inputController->notify();
  }
}

void BleController::press(uint8_t b) {
  uint8_t index = (b - 1) / 8;
  uint8_t bit = (b - 1) % 8;
  uint8_t bitmask = (1 << bit);

  uint8_t result = _buttons[index] | bitmask;

  if (result != _buttons[index]) {
    _buttons[index] = result;
  }

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::release(uint8_t b) {
  uint8_t index = (b - 1) / 8;
  uint8_t bit = (b - 1) % 8;
  uint8_t bitmask = (1 << bit);

  uint64_t result = _buttons[index] & ~bitmask;

  if (result != _buttons[index]) {
    _buttons[index] = result;
  }

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

uint8_t BleController::specialButtonBitPosition(uint8_t b) {
  if (b >= POSSIBLESPECIALBUTTONS)
    throw std::invalid_argument("Index out of range");
  uint8_t bit = 0;

  for (int i = 0; i < b; i++) {
    if (configuration.getWhichSpecialButtons()[i])
      bit++;
  }
  return bit;
}

void BleController::pressSpecialButton(uint8_t b) {
  uint8_t button = specialButtonBitPosition(b);
  uint8_t bit = button % 8;
  uint8_t bitmask = (1 << bit);

  uint64_t result = _specialButtons | bitmask;

  if (result != _specialButtons) {
    _specialButtons = result;
  }

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::releaseSpecialButton(uint8_t b) {
  uint8_t button = specialButtonBitPosition(b);
  uint8_t bit = button % 8;
  uint8_t bitmask = (1 << bit);

  uint64_t result = _specialButtons & ~bitmask;

  if (result != _specialButtons) {
    _specialButtons = result;
  }

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::pressStart() { pressSpecialButton(START_BUTTON); }

void BleController::releaseStart() { releaseSpecialButton(START_BUTTON); }

void BleController::pressSelect() { pressSpecialButton(SELECT_BUTTON); }

void BleController::releaseSelect() { releaseSpecialButton(SELECT_BUTTON); }

void BleController::pressMenu() { pressSpecialButton(MENU_BUTTON); }

void BleController::releaseMenu() { releaseSpecialButton(MENU_BUTTON); }

void BleController::pressHome() { pressSpecialButton(HOME_BUTTON); }

void BleController::releaseHome() { releaseSpecialButton(HOME_BUTTON); }

void BleController::pressBack() { pressSpecialButton(BACK_BUTTON); }

void BleController::releaseBack() { releaseSpecialButton(BACK_BUTTON); }

void BleController::pressVolumeInc() { pressSpecialButton(VOLUME_INC_BUTTON); }

void BleController::releaseVolumeInc() {
  releaseSpecialButton(VOLUME_INC_BUTTON);
}

void BleController::pressVolumeDec() { pressSpecialButton(VOLUME_DEC_BUTTON); }

void BleController::releaseVolumeDec() {
  releaseSpecialButton(VOLUME_DEC_BUTTON);
}

void BleController::pressVolumeMute() {
  pressSpecialButton(VOLUME_MUTE_BUTTON);
}

void BleController::releaseVolumeMute() {
  releaseSpecialButton(VOLUME_MUTE_BUTTON);
}

void BleController::setLeftThumb(int16_t x, int16_t y) {
  if (x == -32768) {
    x = -32767;
  }
  if (y == -32768) {
    y = -32767;
  }

  _x = x;
  _y = y;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setRightThumb(int16_t z, int16_t rZ) {
  if (z == -32768) {
    z = -32767;
  }
  if (rZ == -32768) {
    rZ = -32767;
  }

  _z = z;
  _rZ = rZ;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setRightThumbAndroid(int16_t z, int16_t rX) {
  if (z == -32768) {
    z = -32767;
  }
  if (rX == -32768) {
    rX = -32767;
  }

  _z = z;
  _rX = rX;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setLeftTrigger(int16_t rX) {
  if (rX == -32768) {
    rX = -32767;
  }

  _rX = rX;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setRightTrigger(int16_t rY) {
  if (rY == -32768) {
    rY = -32767;
  }

  _rY = rY;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setTriggers(int16_t rX, int16_t rY) {
  if (rX == -32768) {
    rX = -32767;
  }
  if (rY == -32768) {
    rY = -32767;
  }

  _rX = rX;
  _rY = rY;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setHat(signed char hat) {
  _hat1 = hat;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setHat1(signed char hat1) {
  _hat1 = hat1;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setHat2(signed char hat2) {
  _hat2 = hat2;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setHat3(signed char hat3) {
  _hat3 = hat3;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setHat4(signed char hat4) {
  _hat4 = hat4;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setX(int16_t x) {
  if (x == -32768) {
    x = -32767;
  }

  _x = x;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setY(int16_t y) {
  if (y == -32768) {
    y = -32767;
  }

  _y = y;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setZ(int16_t z) {
  if (z == -32768) {
    z = -32767;
  }

  _z = z;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setRZ(int16_t rZ) {
  if (rZ == -32768) {
    rZ = -32767;
  }

  _rZ = rZ;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setRX(int16_t rX) {
  if (rX == -32768) {
    rX = -32767;
  }

  _rX = rX;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setRY(int16_t rY) {
  if (rY == -32768) {
    rY = -32767;
  }

  _rY = rY;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setSlider(int16_t slider) {
  if (slider == -32768) {
    slider = -32767;
  }

  _slider1 = slider;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setSlider1(int16_t slider1) {
  if (slider1 == -32768) {
    slider1 = -32767;
  }

  _slider1 = slider1;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setSlider2(int16_t slider2) {
  if (slider2 == -32768) {
    slider2 = -32767;
  }

  _slider2 = slider2;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setRudder(int16_t rudder) {
  if (rudder == -32768) {
    rudder = -32767;
  }

  _rudder = rudder;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setThrottle(int16_t throttle) {
  if (throttle == -32768) {
    throttle = -32767;
  }

  _throttle = throttle;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setAccelerator(int16_t accelerator) {
  if (accelerator == -32768) {
    accelerator = -32767;
  }

  _accelerator = accelerator;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setBrake(int16_t brake) {
  if (brake == -32768) {
    brake = -32767;
  }

  _brake = brake;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setSteering(int16_t steering) {
  if (steering == -32768) {
    steering = -32767;
  }

  _steering = steering;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

bool BleController::isPressed(uint8_t b) {
  uint8_t index = (b - 1) / 8;
  uint8_t bit = (b - 1) % 8;
  uint8_t bitmask = (1 << bit);

  if ((bitmask & _buttons[index]) > 0)
    return true;
  return false;
}

bool BleController::isConnected(void) {
  return this->connectionStatus->connected;
}

void BleController::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (hid != 0) {

    this->hid->setBatteryLevel(this->batteryLevel,
                               this->isConnected() ? true : false);

    if (configuration.getAutoReport()) {
      sendReport();
    }
  }
}

bool BleController::isOutputReceived() {
  if (enableOutputReport && outputReceiver) {
    if (this->outputReceiver->outputFlag) {
      this->outputReceiver->outputFlag = false; // Clear Flag
      return true;
    }
  }
  return false;
}

uint8_t *BleController::getOutputBuffer() {
  if (enableOutputReport && outputReceiver) {
    memcpy(outputBackupBuffer, outputReceiver->outputBuffer,
           outputReportLength); // Creating a backup to avoid buffer being
                                // overwritten while processing data
    return outputBackupBuffer;
  }
  return nullptr;
}

bool BleController::deleteAllBonds(bool resetBoard) {
  bool success = false;

  NimBLEDevice::deleteAllBonds();
  NIMBLE_LOGD(LOG_TAG, "deleteAllBonds - All bonds deleted");
  success = true;
  delay(500);

  if (resetBoard) {
    NIMBLE_LOGD(LOG_TAG, "deleteAllBonds - Reboot ESP32");
    ESP.restart();
  }

  return success; // Returns false if all bonds are not deleted
}

bool BleController::deleteBond(bool resetBoard) {
  bool success = false;

  NimBLEServer *server = NimBLEDevice::getServer();

  if (server) {
    NimBLEConnInfo info = server->getPeerInfo(0);
    NimBLEAddress address = info.getAddress();

    success = NimBLEDevice::deleteBond(address);
    NIMBLE_LOGD(LOG_TAG, "deleteBond - Bond for %s deleted",
                std::string(address).c_str());

    delay(500);

    if (resetBoard) {
      NIMBLE_LOGD(LOG_TAG, "deleteBond - Reboot ESP32");
      ESP.restart();
    }
  }
  return success; // Returns false if current bond is not deleted
}

bool BleController::enterPairingMode() {
  NimBLEServer *server = NimBLEDevice::getServer();

  if (server) {
    NIMBLE_LOGD(LOG_TAG, "enterPairingMode - Pairing mode entered");

    // Get current connection information and address
    NimBLEConnInfo currentConnInfo = server->getPeerInfo(0);
    NimBLEAddress currentAddress = currentConnInfo.getAddress();
    NIMBLE_LOGD(LOG_TAG, "enterPairingMode - Connected Address: %s",
                std::string(currentAddress).c_str());

    // Disconnect from current connection
    for (uint16_t connHandle : server->getPeerDevices()) {
      server->disconnect(connHandle); // Disconnect the client
      NIMBLE_LOGD(LOG_TAG, "enterPairingMode - Disconnected from client");
      delay(500);
    }

    bool connectedToOldDevice = true;

    // While connected to old device, keep allowing to connect new new devices
    NIMBLE_LOGD(LOG_TAG, "enterPairingMode - Advertising for clients...");
    while (connectedToOldDevice) {
      delay(10); // Needs a delay to work - do not remove!

      if (this->isConnected()) {
        NimBLEConnInfo newConnInfo = server->getPeerInfo(0);
        NimBLEAddress newAddress = newConnInfo.getAddress();

        // Block specific MAC address
        if (newAddress == currentAddress) {
          NIMBLE_LOGD(LOG_TAG,
                      "enterPairingMode - Connected to previous client, so "
                      "disconnect and continue advertising for new client");
          server->disconnect(newConnInfo.getConnHandle());
          delay(500);
        } else {
          NIMBLE_LOGD(LOG_TAG, "enterPairingMode - Connected to new client");
          NIMBLE_LOGD(LOG_TAG, "enterPairingMode - Exit pairing mode");
          connectedToOldDevice = false;
          return true;
        }
      }
    }
    return false; // Might want to adjust this function to stay in pairing mode
                  // for a while, and then return false after a while if no
                  // other device pairs with it
  }
  return false;
}

NimBLEAddress BleController::getAddress() {
  NimBLEServer *server = NimBLEDevice::getServer();

  if (server) {
    // Get current connection information and address
    NimBLEConnInfo currentConnInfo = server->getPeerInfo(0);
    NimBLEAddress currentAddress = currentConnInfo.getAddress();
    return currentAddress;
  }
  NimBLEAddress blankAddress("00:00:00:00:00:00", 0);
  return blankAddress;
}

String BleController::getStringAddress() {
  NimBLEServer *server = NimBLEDevice::getServer();

  if (server) {
    // Get current connection information and address
    NimBLEConnInfo currentConnInfo = server->getPeerInfo(0);
    NimBLEAddress currentAddress = currentConnInfo.getAddress();
    return currentAddress.toString().c_str();
  }
  NimBLEAddress blankAddress("00:00:00:00:00:00", 0);
  return blankAddress.toString().c_str();
}

NimBLEConnInfo BleController::getPeerInfo() {
  NimBLEServer *server = NimBLEDevice::getServer();
  NimBLEConnInfo currentConnInfo = server->getPeerInfo(0);
  return currentConnInfo;
}

String BleController::getDeviceName() { return this->deviceName.c_str(); }

String BleController::getDeviceManufacturer() {
  return this->deviceManufacturer.c_str();
}

int8_t BleController::getTXPowerLevel() { return NimBLEDevice::getPower(); }

void BleController::setTXPowerLevel(int8_t level) {
  NimBLEDevice::setPower(
      level); // The only valid values are: -12, -9, -6, -3, 0, 3, 6 and 9
  configuration.setTXPowerLevel(level);
}

void BleController::setGyroscope(int16_t gX, int16_t gY, int16_t gZ) {
  if (gX == -32768) {
    gX = -32767;
  }

  if (gY == -32768) {
    gY = -32767;
  }

  if (gY == -32768) {
    gY = -32767;
  }

  _gX = gX;
  _gY = gY;
  _gZ = gZ;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setAccelerometer(int16_t aX, int16_t aY, int16_t aZ) {
  _aX = aX;
  _aY = aY;
  _aZ = aZ;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setMotionControls(int16_t gX, int16_t gY, int16_t gZ,
                                      int16_t aX, int16_t aY, int16_t aZ) {
  if (gX == -32768) {
    gX = -32767;
  }

  if (gY == -32768) {
    gY = -32767;
  }

  if (gZ == -32768) {
    gZ = -32767;
  }

  if (aX == -32768) {
    aX = -32767;
  }

  if (aY == -32768) {
    aY = -32767;
  }

  if (aZ == -32768) {
    aZ = -32767;
  }

  _gX = gX;
  _gY = gY;
  _gZ = gZ;
  _aX = aX;
  _aY = aY;
  _aZ = aZ;

  if (configuration.getAutoReport()) {
    sendReport();
  }
}

void BleController::setPowerStateAll(uint8_t batteryPowerInformation,
                                     uint8_t dischargingState,
                                     uint8_t chargingState,
                                     uint8_t powerLevel) {
  uint8_t powerStateBits = 0b00000000;

  _batteryPowerInformation = batteryPowerInformation;
  _dischargingState = dischargingState;
  _chargingState = chargingState;
  _powerLevel = powerLevel;

  // HID Battery Power State Bits:
  // Bits 0 and 1: Battery Power Information : 0(0b00) = Unknown, 1(0b01) = Not
  // Supported,  2(0b10) = Not Present,               3(0b11) = Present Bits 2
  // and 3: Discharging State         : 0(0b00) = Unknown, 1(0b01) = Not
  // Supported,  2(0b10) = Not Discharging,           3(0b11) = Discharging Bits
  // 4 and 5: Charging State            : 0(0b00) = Unknown, 1(0b01) = Not
  // Chargeable, 2(0b10) = Not Charging (Chargeable), 3(0b11) = Charging
  // (Chargeable) Bits 6 and 7: Power Level               : 0(0b00) = Unknown,
  // 1(0b01) = Not Supported,  2(0b10) = Good Level,                3(0b11) =
  // Critically Low Level

  powerStateBits |=
      (_batteryPowerInformation << 0); // Populate first 2 bits with data
  powerStateBits |=
      (_dischargingState << 2);            // Populate second 2 bits with data
  powerStateBits |= (_chargingState << 4); // Populate third 2 bits with data
  powerStateBits |= (_powerLevel << 6);    // Populate last 2 bits with data

  if (this->pCharacteristic_Power_State) {
    this->pCharacteristic_Power_State->setValue(&powerStateBits, 1);
    this->pCharacteristic_Power_State->notify();
  }
}

void BleController::setBatteryPowerInformation(
    uint8_t batteryPowerInformation) {
  _batteryPowerInformation = batteryPowerInformation;
  setPowerStateAll(_batteryPowerInformation, _dischargingState, _chargingState,
                   _powerLevel);
}

void BleController::setDischargingState(uint8_t dischargingState) {
  _dischargingState = dischargingState;
  setPowerStateAll(_batteryPowerInformation, _dischargingState, _chargingState,
                   _powerLevel);
}

void BleController::setChargingState(uint8_t chargingState) {
  _chargingState = chargingState;
  setPowerStateAll(_batteryPowerInformation, _dischargingState, _chargingState,
                   _powerLevel);
}

void BleController::setPowerLevel(uint8_t powerLevel) {
  _powerLevel = powerLevel;
  setPowerStateAll(_batteryPowerInformation, _dischargingState, _chargingState,
                   _powerLevel);
}

#if BLE_CONTROLLER_DEBUG == 1
static void dumpHidReportDescriptor(const uint8_t *desc, size_t size) {
  if (!Serial) {
    // Serial not initialized yet, avoid printing
    return;
  }

  if (desc == nullptr || size == 0) {
    Serial.println(
        "[BleController][ERROR] HID Report Descriptor is null or empty!");
    return;
  }

  Serial.printf("[BleController][INFO] HID Report Descriptor size: %u bytes\n",
                (unsigned)size);
  for (size_t i = 0; i < size; i++) {
    if (i % 16 == 0) {
      Serial.printf("\n%03u: ", (unsigned)i);
    }
    Serial.printf("%02X ", desc[i]);
  }
  Serial.println("\n[BleController][INFO] End of HID Report Descriptor");

  Serial.printf("\n\nCopy start under here\n");
  for (size_t i = 0; i < size; i++) {
    if (i % 16 == 0) {
      Serial.printf("\n");
    }
    Serial.printf("%02X ", desc[i]);
  }
  Serial.println("\nCopy end above here ");
  Serial.println("\n\nCopy and paste the output above and use a parser such as "
                 "at https://eleccelerator.com/usbdescreqparser to create a "
                 "readable HID Report Descriptor\n\n");
}

static void dumpHIDReport(const uint8_t *report, size_t size) {
  if (!Serial) {
    // Serial not initialized yet, avoid printing
    return;
  }

  Serial.printf("[BleController][INFO] HID Report Dump size: %u bytes\n",
                (unsigned)size);
  for (size_t i = 0; i < size; i++) {
    Serial.printf("%02X ", report[i]);
    // Optional: break line every 16 bytes
    if ((i + 1) % 16 == 0)
      Serial.println();
  }
  Serial.println();
  Serial.println("\n[BleController][INFO] End of HID Report Dump");
}
#endif

void BleController::beginNUS() {
  if (!this->nusInitialized) {
    // Extrememly important to make sure that the pointer to server is actually
    // valid
    while (!NimBLEDevice::isInitialized()) {
    } // Wait until the server is initialized
    while (NimBLEDevice::getServer() == nullptr) {
    } // Ensure pointer to server is actually valid

    // Now server is nkown to be valid, initialise nus to new BleNUS instance
    nus = new BleNUS(NimBLEDevice::getServer()); // Pass the existing BLE server
    nus->begin();
    nusInitialized = true;
  }
}

BleNUS *BleController::getNUS() {
  return nus; // Return a pointer instead of a reference
}

void BleController::sendDataOverNUS(const uint8_t *data, size_t length) {
  if (nus) {
    nus->sendData(data, length);
  }
}

void BleController::setNUSDataReceivedCallback(
    void (*callback)(const uint8_t *data, size_t length)) {
  if (nus) {
    nus->setDataReceivedCallback(callback);
  }
}

void BleController::taskServer(void *pvParameter) {
  BleController *BleControllerInstance = (BleController *)
      pvParameter; // static_cast<BleController *>(pvParameter);

  NimBLEDevice::init(BleControllerInstance->deviceName);
  NimBLEDevice::setPower(
      BleControllerInstance->configuration
          .getTXPowerLevel()); // Set transmit power for advertising (Range: -12
                               // to +9 dBm)
  NimBLEServer *pServer = NimBLEDevice::createServer();

  pServer->setCallbacks(BleControllerInstance->connectionStatus);
  pServer->advertiseOnDisconnect(true);

  BleControllerInstance->hid = new NimBLEHIDDevice(pServer);

  // Initialize gamepad input characteristic if enabled
  if (BleControllerInstance->configuration.getGamepadEnabled()) {
    BleControllerInstance->inputController =
        BleControllerInstance->hid->getInputReport(
            BleControllerInstance->configuration
                .getHidReportId()); // <-- input REPORTID from report map
    BleControllerInstance->connectionStatus->inputController =
        BleControllerInstance->inputController;
  }

  // Initialize keyboard input characteristic if enabled
  if (BleControllerInstance->configuration.getKeyboardEnabled()) {
    BleControllerInstance->inputKeyboard =
        BleControllerInstance->hid->getInputReport(KEYBOARD_REPORT_ID);
  }

  // Initialize mouse input characteristic if enabled
  if (BleControllerInstance->configuration.getMouseEnabled()) {
    BleControllerInstance->inputMouse =
        BleControllerInstance->hid->getInputReport(MOUSE_REPORT_ID);
  }

  if (BleControllerInstance->enableOutputReport) {
    BleControllerInstance->outputController =
        BleControllerInstance->hid->getOutputReport(
            BleControllerInstance->configuration.getHidReportId());
    BleControllerInstance->outputReceiver =
        new BleOutputReceiver(BleControllerInstance->outputReportLength);
    BleControllerInstance->outputBackupBuffer =
        new uint8_t[BleControllerInstance->outputReportLength];
    BleControllerInstance->outputController->setCallbacks(
        BleControllerInstance->outputReceiver);
  }

  BleControllerInstance->hid->setManufacturer(
      BleControllerInstance->deviceManufacturer);

  NimBLEService *pService =
      pServer->getServiceByUUID(SERVICE_UUID_DEVICE_INFORMATION);

  BLECharacteristic *pCharacteristic_Model_Number =
      pService->createCharacteristic(CHARACTERISTIC_UUID_MODEL_NUMBER,
                                     NIMBLE_PROPERTY::READ);
  pCharacteristic_Model_Number->setValue(
      std::string(BleControllerInstance->configuration.getModelNumber()));

  BLECharacteristic *pCharacteristic_Software_Revision =
      pService->createCharacteristic(CHARACTERISTIC_UUID_SOFTWARE_REVISION,
                                     NIMBLE_PROPERTY::READ);
  pCharacteristic_Software_Revision->setValue(
      std::string(BleControllerInstance->configuration.getSoftwareRevision()));

  BLECharacteristic *pCharacteristic_Serial_Number =
      pService->createCharacteristic(CHARACTERISTIC_UUID_SERIAL_NUMBER,
                                     NIMBLE_PROPERTY::READ);
  pCharacteristic_Serial_Number->setValue(
      std::string(BleControllerInstance->configuration.getSerialNumber()));

  BLECharacteristic *pCharacteristic_Firmware_Revision =
      pService->createCharacteristic(CHARACTERISTIC_UUID_FIRMWARE_REVISION,
                                     NIMBLE_PROPERTY::READ);
  pCharacteristic_Firmware_Revision->setValue(
      std::string(BleControllerInstance->configuration.getFirmwareRevision()));

  BLECharacteristic *pCharacteristic_Hardware_Revision =
      pService->createCharacteristic(CHARACTERISTIC_UUID_HARDWARE_REVISION,
                                     NIMBLE_PROPERTY::READ);
  pCharacteristic_Hardware_Revision->setValue(
      std::string(BleControllerInstance->configuration.getHardwareRevision()));

  NimBLECharacteristic *pCharacteristic_Power_State =
      BleControllerInstance->hid->getBatteryService()->createCharacteristic(
          CHARACTERISTIC_UUID_BATTERY_POWER_STATE,
          NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  BleControllerInstance->pCharacteristic_Power_State =
      pCharacteristic_Power_State; // Assign the created characteristic
  BleControllerInstance->pCharacteristic_Power_State->setValue(
      0b00000000); // Now it's safe to call setValue <- Set all to unknown by
                   // default

  BleControllerInstance->hid->setPnp(
      0x01, BleControllerInstance->configuration.getVid(),
      BleControllerInstance->configuration.getPid(),
      BleControllerInstance->configuration.getGuidVersion());
  BleControllerInstance->hid->setHidInfo(0x00, 0x01);

  // NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::setSecurityAuth(true, false,
                                false); // enable bonding, no MITM, no SC

  uint8_t *customHidReportDescriptor =
      new uint8_t[BleControllerInstance->hidReportDescriptorSize];
  memcpy(customHidReportDescriptor,
         BleControllerInstance->tempHidReportDescriptor,
         BleControllerInstance->hidReportDescriptorSize);

#if BLE_CONTROLLER_DEBUG == 1
  // Print HidReportDescriptor to Serial
  dumpHidReportDescriptor(BleControllerInstance->tempHidReportDescriptor,
                          BleControllerInstance->hidReportDescriptorSize);
#endif

  BleControllerInstance->hid->setReportMap(
      (uint8_t *)customHidReportDescriptor,
      BleControllerInstance->hidReportDescriptorSize);
  BleControllerInstance->hid->startServices();

  BleControllerInstance->onStarted(pServer);

  NimBLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_CONTROLLER);
  pAdvertising->setName(BleControllerInstance->deviceName);
  pAdvertising->addServiceUUID(
      BleControllerInstance->hid->getHidService()->getUUID());

  if (BleControllerInstance->delayAdvertising) {
    NIMBLE_LOGD(LOG_TAG, "Main NimBLE server advertising delayed (until Nordic "
                         "UART Service added)");
  } else {
    NIMBLE_LOGD(LOG_TAG, "Main NimBLE server advertising started!");
    pAdvertising->start();
  }

  BleControllerInstance->hid->setBatteryLevel(
      BleControllerInstance->batteryLevel);

  vTaskDelay(portMAX_DELAY); // delay(portMAX_DELAY);
}

// ===================== KEYBOARD METHODS =====================
// Completely rewritten for reliable operation
// Based on ESP32-NimBLE-Keyboard reference implementation

// Internal helper: send a raw keyboard HID report
// Report format: 8 bytes (NO report ID - NimBLE adds it internally)
// [modifiers, reserved, key1, key2, key3, key4, key5, key6]
void BleController::sendRawKeyboard(uint8_t modifiers, uint8_t key1,
                                    uint8_t key2, uint8_t key3, uint8_t key4,
                                    uint8_t key5, uint8_t key6) {
  if (!this->isConnected())
    return;

  // 8 bytes: modifiers + reserved + 6 keys (NO report ID!)
  uint8_t report[8];
  report[0] = modifiers;
  report[1] = 0x00; // Reserved
  report[2] = key1;
  report[3] = key2;
  report[4] = key3;
  report[5] = key4;
  report[6] = key5;
  report[7] = key6;

  this->inputKeyboard->setValue(report, sizeof(report));
  this->inputKeyboard->notify();
}

void BleController::keyboardPress(uint8_t key) {
  if (!this->isConnected())
    return;

  // Add key to the report if not already present
  for (int i = 0; i < 6; i++) {
    if (_keyboardReport.keys[i] == key) {
      return; // Key already pressed
    }
    if (_keyboardReport.keys[i] == 0) {
      _keyboardReport.keys[i] = key;
      break;
    }
  }
  sendKeyboardReport();
}

void BleController::keyboardRelease(uint8_t key) {
  if (!this->isConnected())
    return;

  // Remove key from the report
  for (int i = 0; i < 6; i++) {
    if (_keyboardReport.keys[i] == key) {
      _keyboardReport.keys[i] = 0;
      // Shift remaining keys down
      for (int j = i; j < 5; j++) {
        _keyboardReport.keys[j] = _keyboardReport.keys[j + 1];
      }
      _keyboardReport.keys[5] = 0;
      break;
    }
  }
  sendKeyboardReport();
}

void BleController::keyboardReleaseAll() {
  if (!this->isConnected())
    return;

  // Zero out everything and send
  memset(&_keyboardReport, 0, sizeof(_keyboardReport));
  sendRawKeyboard(0, 0, 0, 0, 0, 0, 0);
}

// Type a single HID key code (press and release)
void BleController::keyboardWrite(uint8_t key) {
  if (!this->isConnected())
    return;

  // Press
  sendRawKeyboard(0, key, 0, 0, 0, 0, 0);
  delay(15);
  // Release
  sendRawKeyboard(0, 0, 0, 0, 0, 0, 0);
  delay(15);
}

// Type a single character with proper shift handling
void BleController::typeChar(char c) {
  if (!this->isConnected())
    return;

  uint8_t hidKey = asciiToHID(c);
  if (hidKey == 0)
    return;

  uint8_t modifier = needsShift(c) ? KEY_MOD_LSHIFT : 0;

  // Press key with modifier
  sendRawKeyboard(modifier, hidKey, 0, 0, 0, 0, 0);
  delay(15);
  // Release everything
  sendRawKeyboard(0, 0, 0, 0, 0, 0, 0);
  delay(15);
}

void BleController::keyboardWrite(const char *str) {
  if (!this->isConnected())
    return;

  for (int i = 0; str[i] != '\0'; i++) {
    typeChar(str[i]);
  }
}

void BleController::keyboardPrint(const char *str) { keyboardWrite(str); }

void BleController::keyboardPrint(String str) { keyboardWrite(str.c_str()); }

void BleController::setKeyboardModifiers(uint8_t modifiers) {
  _keyboardReport.modifiers = modifiers;
  sendKeyboardReport();
}

void BleController::sendKeyboardReport() {
  if (!this->isConnected())
    return;

  sendRawKeyboard(_keyboardReport.modifiers, _keyboardReport.keys[0],
                  _keyboardReport.keys[1], _keyboardReport.keys[2],
                  _keyboardReport.keys[3], _keyboardReport.keys[4],
                  _keyboardReport.keys[5]);
}

void BleController::rawKeyboardAction(uint8_t msg[], char msgSize) {
  if (!this->isConnected())
    return;

  this->inputKeyboard->setValue(msg, msgSize);
  this->inputKeyboard->notify();
}

// ===================== MOUSE METHODS =====================
// Based on ESP32-NimBLE-Mouse reference implementation
// Report format: [buttons, x, y, wheel, hWheel] = 5 bytes
// NimBLE adds Report ID internally for getInputReport(REPORT_ID)

void BleController::sendRawMouse(uint8_t buttons, int8_t x, int8_t y,
                                 int8_t wheel) {
  if (!this->isConnected())
    return;

  // 5 bytes only - NO Report ID (NimBLE adds it internally)
  // Format: [buttons, x, y, wheel, hWheel]
  uint8_t m[5];
  m[0] = buttons;
  m[1] = x;
  m[2] = y;
  m[3] = wheel;
  m[4] = 0; // Horizontal wheel (not used)

  this->inputMouse->setValue(m, 5);
  this->inputMouse->notify();
}

void BleController::mouseClick(uint8_t button) {
  if (!this->isConnected())
    return;

  // Press
  sendRawMouse(button, 0, 0, 0);
  delay(15);
  // Release
  sendRawMouse(0, 0, 0, 0);
  delay(15);
}

void BleController::mousePress(uint8_t button) {
  if (!this->isConnected())
    return;

  _mouseReport.buttons |= button;
  sendRawMouse(_mouseReport.buttons, 0, 0, 0);
}

void BleController::mouseRelease(uint8_t button) {
  if (!this->isConnected())
    return;

  _mouseReport.buttons &= ~button;
  sendRawMouse(_mouseReport.buttons, 0, 0, 0);
}

void BleController::mouseReleaseAll() {
  if (!this->isConnected())
    return;

  _mouseReport.buttons = 0;
  _mouseReport.x = 0;
  _mouseReport.y = 0;
  _mouseReport.wheel = 0;
  sendRawMouse(0, 0, 0, 0);
}

void BleController::mouseMove(int8_t x, int8_t y) {
  if (!this->isConnected())
    return;

  // Send movement directly - relative mouse needs fresh movement each time
  sendRawMouse(_mouseReport.buttons, x, y, 0);
}

void BleController::mouseScroll(int8_t scroll) {
  if (!this->isConnected())
    return;

  sendRawMouse(_mouseReport.buttons, 0, 0, scroll);
}

void BleController::sendMouseReport() {
  if (!this->isConnected())
    return;

  sendRawMouse(_mouseReport.buttons, _mouseReport.x, _mouseReport.y,
               _mouseReport.wheel);
}

void BleController::rawMouseAction(uint8_t msg[], char msgSize) {
  if (!this->isConnected())
    return;

  this->inputMouse->setValue(msg, msgSize);
  this->inputMouse->notify();

#if BLE_CONTROLLER_DEBUG == 1
  dumpHIDReport(msg, msgSize);
#endif
}

// ASCII to HID scan code conversion for printable characters
// For shifted characters (!@#$ etc), returns the base key HID code
// The needsShift() function determines if SHIFT modifier is needed
uint8_t asciiToHID(char ascii) {
  // Lowercase letters a-z
  if (ascii >= 'a' && ascii <= 'z') {
    return ascii - 'a' + 0x04; // a-z -> 0x04-0x1D
  }
  // Uppercase letters A-Z (same HID code as lowercase, shift handled by
  // modifier)
  if (ascii >= 'A' && ascii <= 'Z') {
    return ascii - 'A' + 0x04;
  }
  // Numbers 1-9
  if (ascii >= '1' && ascii <= '9') {
    return ascii - '1' + 0x1E; // 1-9 -> 0x1E-0x26
  }

  switch (ascii) {
  // Number row - base keys
  case '0':
    return 0x27;
  case '1':
    return 0x1E;
  case '2':
    return 0x1F;
  case '3':
    return 0x20;
  case '4':
    return 0x21;
  case '5':
    return 0x22;
  case '6':
    return 0x23;
  case '7':
    return 0x24;
  case '8':
    return 0x25;
  case '9':
    return 0x26;

  // Shifted number row symbols - return base key HID code
  case '!':
    return 0x1E; // Shift+1
  case '@':
    return 0x1F; // Shift+2
  case '#':
    return 0x20; // Shift+3
  case '$':
    return 0x21; // Shift+4
  case '%':
    return 0x22; // Shift+5
  case '^':
    return 0x23; // Shift+6
  case '&':
    return 0x24; // Shift+7
  case '*':
    return 0x25; // Shift+8
  case '(':
    return 0x26; // Shift+9
  case ')':
    return 0x27; // Shift+0

  // Punctuation keys - base
  case '-':
    return 0x2D; // Minus
  case '=':
    return 0x2E; // Equal
  case '[':
    return 0x2F; // Left bracket
  case ']':
    return 0x30; // Right bracket
  case '\\':
    return 0x31; // Backslash
  case ';':
    return 0x33; // Semicolon
  case '\'':
    return 0x34; // Apostrophe
  case '`':
    return 0x35; // Grave accent
  case ',':
    return 0x36; // Comma
  case '.':
    return 0x37; // Period
  case '/':
    return 0x38; // Slash

  // Shifted punctuation - return base key HID code
  case '_':
    return 0x2D; // Shift+Minus
  case '+':
    return 0x2E; // Shift+Equal
  case '{':
    return 0x2F; // Shift+[
  case '}':
    return 0x30; // Shift+]
  case '|':
    return 0x31; // Shift+Backslash
  case ':':
    return 0x33; // Shift+Semicolon
  case '"':
    return 0x34; // Shift+Apostrophe
  case '~':
    return 0x35; // Shift+Grave
  case '<':
    return 0x36; // Shift+Comma
  case '>':
    return 0x37; // Shift+Period
  case '?':
    return 0x38; // Shift+Slash

  // Special keys - use raw USB HID codes
  case ' ':
    return 0x2C; // Space
  case '\n':
    return 0x28; // Enter (USB HID code)
  case '\r':
    return 0x28; // Enter
  case '\t':
    return 0x2B; // Tab (USB HID code)
  case '\b':
    return 0x2A; // Backspace (USB HID code)

  default:
    return 0x00; // No key / unknown
  }
}

// Check if character needs shift modifier
bool needsShift(char ascii) {
  // Uppercase letters
  if (ascii >= 'A' && ascii <= 'Z')
    return true;

  // Shifted symbols
  switch (ascii) {
  case '!':
  case '@':
  case '#':
  case '$':
  case '%':
  case '^':
  case '&':
  case '*':
  case '(':
  case ')':
  case '_':
  case '+':
  case '{':
  case '}':
  case '|':
  case ':':
  case '"':
  case '~':
  case '<':
  case '>':
  case '?':
    return true;
  default:
    return false;
  }
}
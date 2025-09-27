#ifndef ESP32_BLE_CONTROLLER_H
#define ESP32_BLE_CONTROLLER_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include "nimconfig.h"
#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include "BleConnectionStatus.h"
#include "BleControllerConfiguration.h"
#include "BleNUS.h"
#include "BleOutputReceiver.h"
#include "NimBLECharacteristic.h"
#include "NimBLEHIDDevice.h"

// Debug enabled, disabled by default
#ifndef BLE_CONTROLLER_DEBUG
#define BLE_CONTROLLER_DEBUG 0
#endif

// Report IDs for multi-functional HID device
#define CONTROLLER_REPORT_ID 0x01
#define KEYBOARD_REPORT_ID 0x02
#define MOUSE_REPORT_ID 0x03

// Keyboard modifier keys
#define KEY_MOD_LCTRL 0x01
#define KEY_MOD_LSHIFT 0x02
#define KEY_MOD_LALT 0x04
#define KEY_MOD_LMETA 0x08
#define KEY_MOD_RCTRL 0x10
#define KEY_MOD_RSHIFT 0x20
#define KEY_MOD_RALT 0x40
#define KEY_MOD_RMETA 0x80

// Mouse button definitions
#define MOUSE_LEFT 0x01
#define MOUSE_RIGHT 0x02
#define MOUSE_MIDDLE 0x04

// Keyboard report structure (8 bytes)
typedef struct {
  uint8_t modifiers; // Modifier keys (Ctrl, Shift, Alt, etc.)
  uint8_t reserved;  // Reserved byte
  uint8_t keys[6];   // Up to 6 simultaneous key presses
} keyboard_report_t;

// Mouse report structure (4 bytes)
typedef struct {
  uint8_t buttons; // Mouse button states
  int8_t x;        // X axis movement
  int8_t y;        // Y axis movement
  int8_t wheel;    // Scroll wheel movement
} mouse_report_t;

class BleController {
private:
  std::string deviceManufacturer;
  std::string deviceName;
  uint8_t tempHidReportDescriptor[300]; // Increased size for multi-device
                                        // descriptor
  int hidReportDescriptorSize;
  uint8_t hidReportSize;
  uint8_t numOfButtonBytes;
  bool enableOutputReport;
  uint16_t outputReportLength;
  uint8_t _buttons[16]; // 8 bits x 16 bytes = 128 bits --> 128 button max
  uint8_t _specialButtons;
  int16_t _x;
  int16_t _y;
  int16_t _z;
  int16_t _rX;
  int16_t _rY;
  int16_t _rZ;
  int16_t _slider1;
  int16_t _slider2;
  int16_t _rudder;
  int16_t _throttle;
  int16_t _accelerator;
  int16_t _brake;
  int16_t _steering;
  int16_t _hat1;
  int16_t _hat2;
  int16_t _hat3;
  int16_t _hat4;
  int16_t _gX;
  int16_t _gY;
  int16_t _gZ;
  int16_t _aX;
  int16_t _aY;
  int16_t _aZ;
  uint8_t _batteryPowerInformation;
  uint8_t _dischargingState;
  uint8_t _chargingState;
  uint8_t _powerLevel;
  bool nusInitialized;

  // Keyboard and mouse support
  keyboard_report_t _keyboardReport;
  mouse_report_t _mouseReport;

  BleConnectionStatus *connectionStatus;
  BleOutputReceiver *outputReceiver;
  NimBLEServer *pServer;
  BleNUS *nus;

  NimBLEHIDDevice *hid;
  NimBLECharacteristic *inputController;
  NimBLECharacteristic *inputKeyboard;
  NimBLECharacteristic *inputMouse;
  NimBLECharacteristic *outputController;
  NimBLECharacteristic *pCharacteristic_Power_State;

  uint8_t *outputBackupBuffer;

  static void taskServer(void *pvParameter);
  uint8_t specialButtonBitPosition(uint8_t specialButton);

public:
  void rawAction(uint8_t msg[], char msgSize);
  void rawKeyboardAction(uint8_t msg[], char msgSize);
  void rawMouseAction(uint8_t msg[], char msgSize);
  BleControllerConfiguration configuration;

  BleController(std::string deviceName = "ESP32 BLE Controller",
             std::string deviceManufacturer = "Espressif",
             uint8_t batteryLevel = 100, bool delayAdvertising = false);
  void begin(BleControllerConfiguration *config = new BleControllerConfiguration());
  void end(void);
  void setAxes(int16_t x = 0, int16_t y = 0, int16_t z = 0, int16_t rX = 0,
               int16_t rY = 0, int16_t rZ = 0, int16_t slider1 = 0,
               int16_t slider2 = 0);
  void setHIDAxes(int16_t x = 0, int16_t y = 0, int16_t z = 0, int16_t rZ = 0,
                  int16_t rX = 0, int16_t rY = 0, int16_t slider1 = 0,
                  int16_t slider2 = 0);
  void press(uint8_t b = BUTTON_1);   // press BUTTON_1 by default
  void release(uint8_t b = BUTTON_1); // release BUTTON_1 by default
  void pressSpecialButton(uint8_t b);
  void releaseSpecialButton(uint8_t b);
  void pressStart();
  void releaseStart();
  void pressSelect();
  void releaseSelect();
  void pressMenu();
  void releaseMenu();
  void pressHome();
  void releaseHome();
  void pressBack();
  void releaseBack();
  void pressVolumeInc();
  void releaseVolumeInc();
  void pressVolumeDec();
  void releaseVolumeDec();
  void pressVolumeMute();
  void releaseVolumeMute();
  void setLeftThumb(int16_t x = 0, int16_t y = 0);
  void setRightThumb(int16_t z = 0, int16_t rZ = 0);
  void setRightThumbAndroid(int16_t z = 0, int16_t rX = 0);
  void setLeftTrigger(int16_t rX = 0);
  void setRightTrigger(int16_t rY = 0);
  void setTriggers(int16_t rX = 0, int16_t rY = 0);
  void setHats(signed char hat1 = 0, signed char hat2 = 0, signed char hat3 = 0,
               signed char hat4 = 0);
  void setHat(signed char hat = 0);
  void setHat1(signed char hat1 = 0);
  void setHat2(signed char hat2 = 0);
  void setHat3(signed char hat3 = 0);
  void setHat4(signed char hat4 = 0);
  void setX(int16_t x = 0);
  void setY(int16_t y = 0);
  void setZ(int16_t z = 0);
  void setRZ(int16_t rZ = 0);
  void setRX(int16_t rX = 0);
  void setRY(int16_t rY = 0);
  void setSliders(int16_t slider1 = 0, int16_t slider2 = 0);
  void setSlider(int16_t slider = 0);
  void setSlider1(int16_t slider1 = 0);
  void setSlider2(int16_t slider2 = 0);
  void setRudder(int16_t rudder = 0);
  void setThrottle(int16_t throttle = 0);
  void setAccelerator(int16_t accelerator = 0);
  void setBrake(int16_t brake = 0);
  void setSteering(int16_t steering = 0);
  void setSimulationControls(int16_t rudder = 0, int16_t throttle = 0,
                             int16_t accelerator = 0, int16_t brake = 0,
                             int16_t steering = 0);
  void sendReport();
  bool isPressed(uint8_t b = BUTTON_1); // check BUTTON_1 by default
  bool isConnected(void);
  void resetButtons();
  void setBatteryLevel(uint8_t level);
  void setPowerStateAll(uint8_t batteryPowerInformation,
                        uint8_t dischargingState, uint8_t chargingState,
                        uint8_t powerLevel);
  void setBatteryPowerInformation(uint8_t batteryPowerInformation);
  void setDischargingState(uint8_t dischargingState);
  void setChargingState(uint8_t chargingState);
  void setPowerLevel(uint8_t powerLevel);
  void setTXPowerLevel(int8_t level = 9);
  int8_t getTXPowerLevel();
  uint8_t batteryLevel;
  bool delayAdvertising;
  bool isOutputReceived();
  uint8_t *getOutputBuffer();
  bool deleteBond(bool resetBoard = false);
  bool deleteAllBonds(bool resetBoard = false);
  bool enterPairingMode();
  NimBLEAddress getAddress();
  String getStringAddress();
  NimBLEConnInfo getPeerInfo();
  String getDeviceName();
  String getDeviceManufacturer();
  void setGyroscope(int16_t gX = 0, int16_t gY = 0, int16_t gZ = 0);
  void setAccelerometer(int16_t aX = 0, int16_t aY = 0, int16_t aZ = 0);
  void setMotionControls(int16_t gX = 0, int16_t gY = 0, int16_t gZ = 0,
                         int16_t aX = 0, int16_t aY = 0, int16_t aZ = 0);
  void beginNUS();
  void sendDataOverNUS(const uint8_t *data, size_t length);
  void setNUSDataReceivedCallback(void (*callback)(const uint8_t *data,
                                                   size_t length));
  BleNUS *getNUS();

  // Keyboard methods
  void keyboardPress(uint8_t key);
  void keyboardRelease(uint8_t key);
  void keyboardReleaseAll();
  void keyboardWrite(uint8_t key);
  void keyboardWrite(const char *str);
  void keyboardPrint(const char *str);
  void keyboardPrint(String str);
  void setKeyboardModifiers(uint8_t modifiers);
  void sendKeyboardReport();

  // Mouse methods
  void mouseClick(uint8_t button = MOUSE_LEFT);
  void mousePress(uint8_t button = MOUSE_LEFT);
  void mouseRelease(uint8_t button = MOUSE_LEFT);
  void mouseReleaseAll();
  void mouseMove(int8_t x, int8_t y);
  void mouseScroll(int8_t scroll);
  void sendMouseReport();

protected:
  virtual void onStarted(NimBLEServer *pServer) {};
};

uint8_t asciiToHID(char ascii);
bool needsShift(char ascii); 

#endif // CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_CONTROLLER_H
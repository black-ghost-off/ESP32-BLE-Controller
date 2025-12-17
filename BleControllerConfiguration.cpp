#include "BleControllerConfiguration.h"

BleControllerConfiguration::BleControllerConfiguration() : _controllerType(CONTROLLER_TYPE_CONTROLLER),
                                                     _autoReport(true),
                                                     _hidReportId(3),
                                                     _buttonCount(16),
                                                     _hatSwitchCount(1),
                                                     _whichSpecialButtons{false, false, false, false, false, false, false, false},
                                                     _whichAxes{true, true, true, true, true, true, true, true},
                                                     _whichSimulationControls{false, false, false, false, false},
                                                     _includeGyroscope(false),
                                                     _includeAccelerometer(false),
                                                     _vid(0xe502),
                                                     _pid(0xbbab),
                                                     _guidVersion(0x0110),
                                                     _axesMin(0x0000),
                                                     _axesMax(0x7FFF),
                                                     _simulationMin(0x0000),
                                                     _simulationMax(0x7FFF),
                                                     _motionMin(0x0000),
                                                     _motionMax(0x7FFF),
                                                     _modelNumber("1.0.0"),
                                                     _softwareRevision("1.0.0"),
                                                     _serialNumber("0123456789"),
                                                     _firmwareRevision("0.7.4"),
                                                     _hardwareRevision("1.0.0"),
                                                     _enableOutputReport(false),
                                                     _enableNordicUARTService(false),
                                                     _outputReportLength(64),
                                                     _transmitPowerLevel(9),
                                                     _enabledDevices(DEVICE_ALL),
                                                     _keyboardKeyCount(6),
                                                     _keyboardMediaKeysEnabled(false),
                                                     _mouseButtonCount(5),
                                                     _mouseWheelEnabled(true),
                                                     _mouseHWheelEnabled(true)
{
}

uint8_t BleControllerConfiguration::getTotalSpecialButtonCount()
{
    int count = 0;
    for (int i = 0; i < POSSIBLESPECIALBUTTONS; i++)
    {
        count += (int)_whichSpecialButtons[i];
    }

    return count;
}

uint8_t BleControllerConfiguration::getDesktopSpecialButtonCount()
{
    int count = 0;
    for (int i = 0; i < 3; i++)
    {
        count += (int)_whichSpecialButtons[i];
    }

    return count;
}

uint8_t BleControllerConfiguration::getConsumerSpecialButtonCount()
{
    int count = 0;
    for (int i = 3; i < 8; i++)
    {
        count += (int)_whichSpecialButtons[i];
    }

    return count;
}

uint8_t BleControllerConfiguration::getAxisCount()
{
    int count = 0;
    for (int i = 0; i < POSSIBLEAXES; i++)
    {
        count += (int)_whichAxes[i];
    }

    return count;
}

uint8_t BleControllerConfiguration::getSimulationCount()
{
    int count = 0;
    for (int i = 0; i < POSSIBLESIMULATIONCONTROLS; i++)
    {
        count += (int)_whichSimulationControls[i];
    }

    return count;
}

uint16_t BleControllerConfiguration::getVid(){ return _vid; }
uint16_t BleControllerConfiguration::getPid(){ return _pid; }
uint16_t BleControllerConfiguration::getGuidVersion(){ return _guidVersion; }
int16_t BleControllerConfiguration::getAxesMin(){ return _axesMin; }
int16_t BleControllerConfiguration::getAxesMax(){ return _axesMax; }
int16_t BleControllerConfiguration::getSimulationMin(){ return _simulationMin; }
int16_t BleControllerConfiguration::getSimulationMax(){ return _simulationMax; }
int16_t BleControllerConfiguration::getMotionMin(){ return _motionMin; }
int16_t BleControllerConfiguration::getMotionMax(){ return _motionMax; }
uint8_t BleControllerConfiguration::getControllerType() { return _controllerType; }
uint8_t BleControllerConfiguration::getHidReportId() { return _hidReportId; }
uint16_t BleControllerConfiguration::getButtonCount() { return _buttonCount; }
uint8_t BleControllerConfiguration::getHatSwitchCount() { return _hatSwitchCount; }
bool BleControllerConfiguration::getAutoReport() { return _autoReport; }
bool BleControllerConfiguration::getIncludeStart() { return _whichSpecialButtons[START_BUTTON]; }
bool BleControllerConfiguration::getIncludeSelect() { return _whichSpecialButtons[SELECT_BUTTON]; }
bool BleControllerConfiguration::getIncludeMenu() { return _whichSpecialButtons[MENU_BUTTON]; }
bool BleControllerConfiguration::getIncludeHome() { return _whichSpecialButtons[HOME_BUTTON]; }
bool BleControllerConfiguration::getIncludeBack() { return _whichSpecialButtons[BACK_BUTTON]; }
bool BleControllerConfiguration::getIncludeVolumeInc() { return _whichSpecialButtons[VOLUME_INC_BUTTON]; }
bool BleControllerConfiguration::getIncludeVolumeDec() { return _whichSpecialButtons[VOLUME_DEC_BUTTON]; }
bool BleControllerConfiguration::getIncludeVolumeMute() { return _whichSpecialButtons[VOLUME_MUTE_BUTTON]; }
const bool *BleControllerConfiguration::getWhichSpecialButtons() const { return _whichSpecialButtons; }
bool BleControllerConfiguration::getIncludeXAxis() { return _whichAxes[X_AXIS]; }
bool BleControllerConfiguration::getIncludeYAxis() { return _whichAxes[Y_AXIS]; }
bool BleControllerConfiguration::getIncludeZAxis() { return _whichAxes[Z_AXIS]; }
bool BleControllerConfiguration::getIncludeRxAxis() { return _whichAxes[RX_AXIS]; }
bool BleControllerConfiguration::getIncludeRyAxis() { return _whichAxes[RY_AXIS]; }
bool BleControllerConfiguration::getIncludeRzAxis() { return _whichAxes[RZ_AXIS]; }
bool BleControllerConfiguration::getIncludeSlider1() { return _whichAxes[SLIDER1]; }
bool BleControllerConfiguration::getIncludeSlider2() { return _whichAxes[SLIDER2]; }
const bool *BleControllerConfiguration::getWhichAxes() const { return _whichAxes; }
bool BleControllerConfiguration::getIncludeRudder() { return _whichSimulationControls[RUDDER]; }
bool BleControllerConfiguration::getIncludeThrottle() { return _whichSimulationControls[THROTTLE]; }
bool BleControllerConfiguration::getIncludeAccelerator() { return _whichSimulationControls[ACCELERATOR]; }
bool BleControllerConfiguration::getIncludeBrake() { return _whichSimulationControls[BRAKE]; }
bool BleControllerConfiguration::getIncludeSteering() { return _whichSimulationControls[STEERING]; }
const bool *BleControllerConfiguration::getWhichSimulationControls() const { return _whichSimulationControls; }
bool BleControllerConfiguration::getIncludeGyroscope() { return _includeGyroscope; }
bool BleControllerConfiguration::getIncludeAccelerometer() { return _includeAccelerometer; }
const char *BleControllerConfiguration::getModelNumber(){ return _modelNumber; }
const char *BleControllerConfiguration::getSoftwareRevision(){ return _softwareRevision; }
const char *BleControllerConfiguration::getSerialNumber(){ return _serialNumber; }
const char *BleControllerConfiguration::getFirmwareRevision(){ return _firmwareRevision; }
const char *BleControllerConfiguration::getHardwareRevision(){ return _hardwareRevision; }
bool BleControllerConfiguration::getEnableOutputReport(){ return _enableOutputReport; }
bool BleControllerConfiguration::getEnableNordicUARTService(){ return _enableNordicUARTService; }
uint16_t BleControllerConfiguration::getOutputReportLength(){ return _outputReportLength; }
int8_t BleControllerConfiguration::getTXPowerLevel(){ return _transmitPowerLevel; }	// Returns the power level that was set as the server started

// Device enable getters
uint8_t BleControllerConfiguration::getEnabledDevices(){ return _enabledDevices; }
bool BleControllerConfiguration::getGamepadEnabled(){ return (_enabledDevices & DEVICE_GAMEPAD) != 0; }
bool BleControllerConfiguration::getKeyboardEnabled(){ return (_enabledDevices & DEVICE_KEYBOARD) != 0; }
bool BleControllerConfiguration::getMouseEnabled(){ return (_enabledDevices & DEVICE_MOUSE) != 0; }

// Keyboard configuration getters
uint8_t BleControllerConfiguration::getKeyboardKeyCount(){ return _keyboardKeyCount; }
bool BleControllerConfiguration::getKeyboardMediaKeysEnabled(){ return _keyboardMediaKeysEnabled; }

// Mouse configuration getters
uint8_t BleControllerConfiguration::getMouseButtonCount(){ return _mouseButtonCount; }
bool BleControllerConfiguration::getMouseWheelEnabled(){ return _mouseWheelEnabled; }
bool BleControllerConfiguration::getMouseHWheelEnabled(){ return _mouseHWheelEnabled; }

void BleControllerConfiguration::setWhichSpecialButtons(bool start, bool select, bool menu, bool home, bool back, bool volumeInc, bool volumeDec, bool volumeMute)
{
    _whichSpecialButtons[START_BUTTON] = start;
    _whichSpecialButtons[SELECT_BUTTON] = select;
    _whichSpecialButtons[MENU_BUTTON] = menu;
    _whichSpecialButtons[HOME_BUTTON] = home;
    _whichSpecialButtons[BACK_BUTTON] = back;
    _whichSpecialButtons[VOLUME_INC_BUTTON] = volumeInc;
    _whichSpecialButtons[VOLUME_DEC_BUTTON] = volumeDec;
    _whichSpecialButtons[VOLUME_MUTE_BUTTON] = volumeMute;
}

void BleControllerConfiguration::setWhichAxes(bool xAxis, bool yAxis, bool zAxis, bool rxAxis, bool ryAxis, bool rzAxis, bool slider1, bool slider2)
{
    _whichAxes[X_AXIS] = xAxis;
    _whichAxes[Y_AXIS] = yAxis;
    _whichAxes[Z_AXIS] = zAxis;
    _whichAxes[RZ_AXIS] = rzAxis;
    _whichAxes[RX_AXIS] = rxAxis;
    _whichAxes[RY_AXIS] = ryAxis;
    _whichAxes[SLIDER1] = slider1;
    _whichAxes[SLIDER2] = slider2;
}

void BleControllerConfiguration::setWhichSimulationControls(bool rudder, bool throttle, bool accelerator, bool brake, bool steering)
{
    _whichSimulationControls[RUDDER] = rudder;
    _whichSimulationControls[THROTTLE] = throttle;
    _whichSimulationControls[ACCELERATOR] = accelerator;
    _whichSimulationControls[BRAKE] = brake;
    _whichSimulationControls[STEERING] = steering;
}

void BleControllerConfiguration::setControllerType(uint8_t value) { _controllerType = value; }
void BleControllerConfiguration::setHidReportId(uint8_t value) { _hidReportId = value; }
void BleControllerConfiguration::setButtonCount(uint16_t value) { _buttonCount = value; }
void BleControllerConfiguration::setHatSwitchCount(uint8_t value) { _hatSwitchCount = value; }
void BleControllerConfiguration::setAutoReport(bool value) { _autoReport = value; }
void BleControllerConfiguration::setIncludeStart(bool value) { _whichSpecialButtons[START_BUTTON] = value; }
void BleControllerConfiguration::setIncludeSelect(bool value) { _whichSpecialButtons[SELECT_BUTTON] = value; }
void BleControllerConfiguration::setIncludeMenu(bool value) { _whichSpecialButtons[MENU_BUTTON] = value; }
void BleControllerConfiguration::setIncludeHome(bool value) { _whichSpecialButtons[HOME_BUTTON] = value; }
void BleControllerConfiguration::setIncludeBack(bool value) { _whichSpecialButtons[BACK_BUTTON] = value; }
void BleControllerConfiguration::setIncludeVolumeInc(bool value) { _whichSpecialButtons[VOLUME_INC_BUTTON] = value; }
void BleControllerConfiguration::setIncludeVolumeDec(bool value) { _whichSpecialButtons[VOLUME_DEC_BUTTON] = value; }
void BleControllerConfiguration::setIncludeVolumeMute(bool value) { _whichSpecialButtons[VOLUME_MUTE_BUTTON] = value; }
void BleControllerConfiguration::setIncludeXAxis(bool value) { _whichAxes[X_AXIS] = value; }
void BleControllerConfiguration::setIncludeYAxis(bool value) { _whichAxes[Y_AXIS] = value; }
void BleControllerConfiguration::setIncludeZAxis(bool value) { _whichAxes[Z_AXIS] = value; }
void BleControllerConfiguration::setIncludeRzAxis(bool value) { _whichAxes[RZ_AXIS] = value; }
void BleControllerConfiguration::setIncludeRxAxis(bool value) { _whichAxes[RX_AXIS] = value; }
void BleControllerConfiguration::setIncludeRyAxis(bool value) { _whichAxes[RY_AXIS] = value; }
void BleControllerConfiguration::setIncludeSlider1(bool value) { _whichAxes[SLIDER1] = value; }
void BleControllerConfiguration::setIncludeSlider2(bool value) { _whichAxes[SLIDER2] = value; }
void BleControllerConfiguration::setIncludeRudder(bool value) { _whichSimulationControls[RUDDER] = value; }
void BleControllerConfiguration::setIncludeThrottle(bool value) { _whichSimulationControls[THROTTLE] = value; }
void BleControllerConfiguration::setIncludeAccelerator(bool value) { _whichSimulationControls[ACCELERATOR] = value; }
void BleControllerConfiguration::setIncludeBrake(bool value) { _whichSimulationControls[BRAKE] = value; }
void BleControllerConfiguration::setIncludeSteering(bool value) { _whichSimulationControls[STEERING] = value; }
void BleControllerConfiguration::setIncludeGyroscope(bool value) { _includeGyroscope = value; }
void BleControllerConfiguration::setIncludeAccelerometer(bool value) { _includeAccelerometer = value; }
void BleControllerConfiguration::setVid(uint16_t value) { _vid = value; }
void BleControllerConfiguration::setPid(uint16_t value) { _pid = value; }
void BleControllerConfiguration::setGuidVersion(uint16_t value) { _guidVersion = value; }
void BleControllerConfiguration::setAxesMin(int16_t value) { _axesMin = value; }
void BleControllerConfiguration::setAxesMax(int16_t value) { _axesMax = value; }
void BleControllerConfiguration::setSimulationMin(int16_t value) { _simulationMin = value; }
void BleControllerConfiguration::setSimulationMax(int16_t value) { _simulationMax = value; }
void BleControllerConfiguration::setMotionMin(int16_t value) { _motionMin = value; }
void BleControllerConfiguration::setMotionMax(int16_t value) { _motionMax = value; }
void BleControllerConfiguration::setModelNumber(const char *value) { _modelNumber = value; }
void BleControllerConfiguration::setSoftwareRevision(const char *value) { _softwareRevision = value; }
void BleControllerConfiguration::setSerialNumber(const char *value) { _serialNumber = value; }
void BleControllerConfiguration::setFirmwareRevision(const char *value) { _firmwareRevision = value; }
void BleControllerConfiguration::setHardwareRevision(const char *value) { _hardwareRevision = value; }
void BleControllerConfiguration::setEnableOutputReport(bool value) { _enableOutputReport = value; }
void BleControllerConfiguration::setEnableNordicUARTService(bool value) { _enableNordicUARTService = value; }
void BleControllerConfiguration::setOutputReportLength(uint16_t value) { _outputReportLength = value; }
void BleControllerConfiguration::setTXPowerLevel(int8_t value) { _transmitPowerLevel = value; }

// Device enable setters
void BleControllerConfiguration::setEnabledDevices(uint8_t devices) { _enabledDevices = devices; }
void BleControllerConfiguration::setGamepadEnabled(bool value) { 
    if (value) _enabledDevices |= DEVICE_GAMEPAD; 
    else _enabledDevices &= ~DEVICE_GAMEPAD; 
}
void BleControllerConfiguration::setKeyboardEnabled(bool value) { 
    if (value) _enabledDevices |= DEVICE_KEYBOARD; 
    else _enabledDevices &= ~DEVICE_KEYBOARD; 
}
void BleControllerConfiguration::setMouseEnabled(bool value) { 
    if (value) _enabledDevices |= DEVICE_MOUSE; 
    else _enabledDevices &= ~DEVICE_MOUSE; 
}

// Keyboard configuration setters
void BleControllerConfiguration::setKeyboardKeyCount(uint8_t count) { 
    _keyboardKeyCount = (count > MAX_KEYBOARD_KEYS) ? MAX_KEYBOARD_KEYS : (count < 1 ? 1 : count); 
}
void BleControllerConfiguration::setKeyboardMediaKeysEnabled(bool value) { _keyboardMediaKeysEnabled = value; }

// Mouse configuration setters
void BleControllerConfiguration::setMouseButtonCount(uint8_t count) { 
    _mouseButtonCount = (count > MAX_MOUSE_BUTTONS) ? MAX_MOUSE_BUTTONS : (count < 1 ? 1 : count); 
}
void BleControllerConfiguration::setMouseWheelEnabled(bool value) { _mouseWheelEnabled = value; }
void BleControllerConfiguration::setMouseHWheelEnabled(bool value) { _mouseHWheelEnabled = value; }

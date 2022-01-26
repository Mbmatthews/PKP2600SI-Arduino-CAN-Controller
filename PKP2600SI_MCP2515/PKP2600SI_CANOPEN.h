/*
 * PKP2600SI Library for controlling it via an MCP2515
 * To configure the keypad in the correct manner, check out 
 * the configuring sketch or follow the guide on the Github 
 * 
 * Created by MBMatthews, December 2021
 */
#include "Arduino.h"
#ifndef TimerOne_h
#include <TimerOne.h>
#endif
#ifndef mcp2515_h
#include <mcp2515.h>
#endif

#ifndef PKP2600_CANOPEN_h
#define PKP2600_CANOPEN_h

//  All these PKP key color definitions are essentially 3 bits long with each bit being R, G, and B
//  Red is 0b100, Green is 0b010, Blue is 0b001 etc.
#define PKP_KEY_BLANK   (0b000)
#define PKP_KEY_RED     (0b100)
#define PKP_KEY_GREEN   (0b010)
#define PKP_KEY_BLUE    (0b001)
#define PKP_KEY_YELLOW  (0b110)
#define PKP_KEY_CYAN    (0b011)
#define PKP_KEY_MAGENTA (0b101)
#define PKP_KEY_WHITE   (0b111)

#define PKP_BACKLIGHT_DEFAULT     (0x00)
#define PKP_BACKLIGHT_RED         (0x01)
#define PKP_BACKLIGHT_GREEN       (0x02)
#define PKP_BACKLIGHT_BLUE        (0x03)
#define PKP_BACKLIGHT_YELLOW      (0x04)
#define PKP_BACKLIGHT_CYAN        (0x05)
#define PKP_BACKLIGHT_VIOLET      (0x06)
#define PKP_BACKLIGHT_WHITE       (0x07)
#define PKP_BACKLIGHT_AMBER       (0x08)
#define PKP_BACKLIGHT_YELLOWGREEN (0x09)

#define PKP_KEY_1  0
#define PKP_KEY_2  1
#define PKP_KEY_3  2
#define PKP_KEY_4  3
#define PKP_KEY_5  4
#define PKP_KEY_6  5
#define PKP_KEY_7  6
#define PKP_KEY_8  7
#define PKP_KEY_9  8
#define PKP_KEY_10 9
#define PKP_KEY_11 10
#define PKP_KEY_12 11

#define BUTTON_MODE_MOMENTARY 1
#define BUTTON_MODE_TOGGLE    2
#define BUTTON_MODE_CYCLE3    3
#define BUTTON_MODE_CYCLE4    4

volatile static bool _mcp_interrupt = false;
volatile static bool _periodicSentFlag=false;
volatile static uint8_t _periodicSendNumber = 0;

class CANKeypad
{
  public:
    // ------ Public Variables ------
    uint8_t keypadCANID = 0x15;
    byte interruptPin = 3;
    uint8_t buttonState[12];
    uint8_t currentBrightness;
    bool interruptAvailable = false;
    struct can_frame rcvMsg;

    // ------ Public Functions ------
    CANKeypad(MCP2515 &mcp1,byte intPin, uint8_t CAN_ID, bool passEnable);
    void begin(CAN_SPEED CANSpeed, CAN_CLOCK mcpClockSpeed);
    void setSerial(Stream *serialReference);
    void process();
    void setKeypadPassword(uint8_t password[4]);
    void setKeyBrightness(uint8_t brightness);
    void setBacklightBrightness(uint8_t color, uint8_t brightness);
    void setKeyColor(uint8_t keyNumber, uint8_t colors[4], uint8_t blinkColors[4]);
    void setKeyMode(uint8_t keyNumber, uint8_t keyMode);
    void setDefaultButtonStates(uint8_t defaultStates[12]);
    void updateKeypad();    


    
  private:
    // ------ Private Variables ------
    MCP2515 &_PKP_MCP;
    Stream *_libSerial;
    struct can_frame _keyColorMsg;
    struct can_frame _backlightBrightnessMsg;
    struct can_frame _keyBrightnessMsg;
    struct can_frame _keyBlinkMsg;
    struct can_frame _keypadMasterStatusMsg;
    struct can_frame _startCANopenNodeMsg;
    bool _buttonPresses[12];
    bool _lastButtonPresses[12];
    uint8_t _buttonMode[12] = {BUTTON_MODE_MOMENTARY,BUTTON_MODE_MOMENTARY,BUTTON_MODE_MOMENTARY,BUTTON_MODE_MOMENTARY,BUTTON_MODE_MOMENTARY,BUTTON_MODE_MOMENTARY,
                              BUTTON_MODE_TOGGLE,BUTTON_MODE_TOGGLE,BUTTON_MODE_TOGGLE,BUTTON_MODE_TOGGLE,BUTTON_MODE_TOGGLE,BUTTON_MODE_TOGGLE};
    bool _currentColors[12][3]  = { {0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0} };
    bool _currentBlinks[12][3]  = { {0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0} };
    bool _keypadUnlocked = false;
    unsigned long _currentMillis=0;
    unsigned long _lastInterruptTime = 0;
    unsigned long _lastKeyReceiveMillis=0; //used for checking the periodic receipt of keys message.  If there isn't any messages for some time it will fault out.
    bool _lastMatch;
    unsigned long _startTime;
    unsigned long _redSetMillis;
    bool _passwordEnable = true;
    uint8_t _currentPass[4];
    uint8_t _keyPass[4];
    byte _keysColor[4][12] = { {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} };
    byte _keysBlinkColor[4][12] = { {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} };
    uint8_t _backlightBrightness = 10;
    uint8_t _defaultBacklightColor = PKP_BACKLIGHT_AMBER;
    uint8_t _keyBrightness = 50;
    uint8_t _defaultButtonState[12] = {0,0,0,0,0,0,1,1,1,1,0,0};
    

    // ------ Private Functions ------
    void setupMessages();
    static void mcpInterruptCallback();
    static void timerOneInterruptCallback();
    void decodeKeys(struct can_frame canMsg);
    void passwordHandler(struct can_frame canMsg);
    void sendKeysStatus();
    void periodicSend();
    void checkForKeypad();
    void keypadWriteColor();
    void keypadWriteBlink();
};




#endif

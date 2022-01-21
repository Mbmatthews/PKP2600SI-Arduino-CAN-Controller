# Arduino-based CAN controller for Blink Marine PKP-2600-SI
This project is a small library used to control a PKP-2600-SI CAN Keypad with an MCP2515. It is structured to be interupt-driven so that the user can do other things in the main loop while the keypad logic is handled in the background.  Messages are sent and received periodically and checked by the library to ensure continuous keypad connectivity.  

Check out my Arduino Automotive CAN Shield project for a custom PCB to control this from an arduino uno, or check out my Arduino CAN input module project, or Arduino CAN relay driver module for examples and easy ways to tie this into your vehicle electronics.  

## Dependencies
This library depends on the following libraries:
- autowp MCP2515 library
- SPI
- TimerOne

## Instructions
Update your keypad to the REV1.5 software.  Blink Marine's programming tool and instructions are included here.  Note that this requires the use of a KVaser or PCAN tool, or some emulator (CANable with PCAN  emulator software for example).

Wire your arduino to the MCP2515 module.  Make sure to use the INT pin from the MCP2515 and make sure that this goes to an Arduino pin that is capable of processing interrupts.

Connect your CAN keypad to the MCP can network and power up the keypad using a power supply (12-24V DC), and make sure you have your 2 120$\Omega$ resistors in place on each end of the CAN bus.  Upload the sketch named "PKP2600SI_Configure.ino" to program the keypad to the desired message format, baudrate, etc.  Then upload the sketch named "PKP2600SI_MCP2515.ino" and watch the keyboard work.  In this sketch you can clearly see how to set up the various key modes, and their colors for each state.  The library uses TimerOne for periodic message transmission, but all other timer functions are left up to the end user to implement in the main loop like the provided sketch does with button 9.

## Library Commands
Here is a list of all the pertinent library commands, although most of this can be gathered by looking at the example sketch.
### Initializer
```cpp
CANKeypad(MCP2515 &mcp1,byte intPin, uint8_t CAN_ID, bool passEnable);
```
Use this command to initializer the CAN keypad object.  User must pass in an MCP2515 object that should be previously declared (the library could be built to contain its own mcp2515 onject but it is done this way so that the user can still process CAN messages and interrupts from the same MCP external to the library, ie in the main loop.)
the **CAN_ID** parameter is the base ID of your CAN keypad.  If you haven't set your keypad's ID to be anything other than the default, then you should just use 0x15, which is the default base ID for the PKP2600SI.
### begin()
```cpp
void begin(CAN_SPEED CANSpeed, CAN_CLOCK mcpClockSpeed);
```
This should be called in the setup function of your sketch.  You must pass in variables of type CAN_SPEED and CAN_CLOCK, which are all defined in the .h file of the MCP2515 library.
CAN_SPEED can be
- CAN_125KBPS,
- CAN_250KBPS
- CAN_500KBPS
- CAN_1000KBP

CAN_CLOCK is the clock speed of your MCP2515 oscillator.  Most are 8 or 16MHz and you should be able to read the crystal oscillator on your MCP2515 module to check. This value can be:
- MCP_8MHZ
- MCP_16MHZ
- MCP_20MHZ

### Serial Reference
```cpp
void setSerial(Stream *serialReference);
```
Mostly used for troubleshooting, and can be very useful if you are modifying this library to do things it wasn't designed to do originally.  Declare a serial object in your setup() and use this command to pass in the serial object to the library for internal use.  Using this looks something like:
```cpp
Serial.begin(115200);
keypad.setSerial(&Serial);
```

### void process();
This is the library function that does all of the actual processing of the keypad messages and updates button states, etc.  This should be placed in your main loop.  **CAUTION:** If you want the arduino to process keypad messages in a relatively quick pace, don't ever use delays in your program, and don't spend too much time in another section of the main loop, else you will experience laggy processing.

### Set Password
```cpp
void setKeypadPassword(uint8_t password[4]);
```
If utilizing a password at boot, this is how you set it.  Pass in an array of your 4 button combination.

### Set Key Brightness
```cpp
void setKeyBrightness(uint8_t brightness);
```
Takes a 0-100 value and sets the keypad brightness.

### Set Backlight Brightness
```cpp
void setBacklightBrightness(uint8_t color, uint8_t brightness);
```
Pass in a backlight color of choice and a brightness of 0-100.
Backlight color options are defined in the .h and are as follows:
- PKP_BACKLIGHT_DEFAULT
- PKP_BACKLIGHT_RED
- PKP_BACKLIGHT_GREEN
- PKP_BACKLIGHT_BLUE
- PKP_BACKLIGHT_YELLOW
- PKP_BACKLIGHT_CYAN
- PKP_BACKLIGHT_VIOLET
- PKP_BACKLIGHT_WHITE
- PKP_BACKLIGHT_AMBER
- PKP_BACKLIGHT_YELLOWGREEN

### Set Key Colors
```cpp
void setKeyColor(uint8_t keyNumber, uint8_t colors[4], uint8_t blinkColors[4]);
```
Key numbers are defined in the .h and are as follows:
- PKP_KEY_1
- PKP_KEY_2
- PKP_KEY_3
- PKP_KEY_4
- PKP_KEY_5
- PKP_KEY_6
- PKP_KEY_7
- PKP_KEY_8
- PKP_KEY_9
- PKP_KEY_10
- PKP_KEY_11
- PKP_KEY_12

Both the colors and blinkColors array and 4 wide arrays of colors for each respective key state.  The key states work like this:
For momentary and latching switches, state=0 is off, state=1 is on.  Your array should specify colors for both states 0 and 1, the other 2 entries of the array can be made to be no color.
For 3-state and 4-state switches, you can use the other slots in these arrays. The blink array works similarly but is what you should use if you want a key to flash on in off while in that state.





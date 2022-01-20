//This sketch sends the proper CAN messages to configure a PKP2600SI in the way that is expected for the actual sketch to control it.  

#include <mcp2515.h>

#define KEYPAD_CAN_ID 0x15
#define PKP_BAUDRATE_125k 0x04
#define PKP_BAUDRATE_250k 0x03
#define PKP_BAUDRATE_500k 0x02
#define PKP_BAUDRATE_1M 0x00

uint8_t keypadBaudrate = PKP_BAUDRATE_250k;
CAN_SPEED mcpBaudrate = CAN_250KBPS; //should be same speed as above
CAN_CLOCK mcpClockSpeed = MCP_8MHZ;


MCP2515 mcp2515(10);

void setup() {
  Serial.begin(115200); 
  //start MCP SPI comms, and check if things are actually working before continuing to config the MCP (when SPI is not working, this always returns ERROR_OK so this doesn't work)
  bool MCP_OK = false;
  while(!MCP_OK){
    if(mcp2515.reset() == MCP2515::ERROR_OK){
      Serial.println("MCP OK");
      MCP_OK = true;
    }
    delay(5);
  }
  mcp2515.setBitrate(mcpBaudrate, mcpClockSpeed);
  mcp2515.setNormalMode();
  keypadConfigure(keypadBaudrate);
}

void loop() {
  // put your main code here, to run repeatedly:

}

void keypadConfigure(uint8_t newBaudrate){
  //This function has delays.  Yes it is sloppy.  However, the keypad will reboot on some commands and 
  //time is needed to not blast messages to the transceiver before it can actually send them all out.
  //Normally using timers is great for doing this efficiently, but this function SHOULD NOT be called
  //often, if ever.  It is included more as an ease of use thing to configure your keypad in the way 
  //the library expects it to be configured so you don't have to manually configure it with a USB-CAN interface.
  //  YMMV

  struct can_frame configMsgExt;

  //Send mesage to change CAN protocol to CANOpen if the current protocol is J1939
  //configMsgExt.ext = true;
  configMsgExt.can_id  = 0x18EF2100;
  configMsgExt.can_dlc = 8;
  configMsgExt.data[0] = 0x04;
  configMsgExt.data[1] = 0x1B;
  configMsgExt.data[2] = 0x80;
  configMsgExt.data[3] = 0x00;
  configMsgExt.data[4] = 0xFF;
  configMsgExt.data[5] = 0xFF;
  configMsgExt.data[6] = 0xFF;
  configMsgExt.data[7] = 0xFF;

  delay(1000);
  
  struct can_frame configurationMsg;

  //send message to configure keypad Baud Rate
  configurationMsg.can_id  = 0x600 + KEYPAD_CAN_ID;
  configurationMsg.can_dlc = 8;
  configurationMsg.data[0] = 0x2F;
  configurationMsg.data[1] = 0x10;
  configurationMsg.data[2] = 0x20;
  configurationMsg.data[3] = 0x00;
  configurationMsg.data[4] = newBaudrate;
  //BYTE 4 configures the baud rate
  //0x00 = 1000k (1M)
  //0x02 = 500k
  //0x03 = 250k
  //0x04 = 125k (default)
  configurationMsg.data[5] = 0x00;
  configurationMsg.data[6] = 0x00;
  configurationMsg.data[7] = 0x00;

  mcp2515.setConfigMode();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  mcp2515.sendMessage(&configurationMsg);
  delay(1000);

  mcp2515.setConfigMode();
  mcp2515.setBitrate(CAN_250KBPS);
  mcp2515.setNormalMode();
  mcp2515.sendMessage(&configurationMsg);
  delay(1000);

  mcp2515.setConfigMode();
  mcp2515.setBitrate(CAN_500KBPS);
  mcp2515.setNormalMode();
  mcp2515.sendMessage(&configurationMsg);
  delay(1000);

  mcp2515.setConfigMode();
  mcp2515.setBitrate(CAN_1000KBPS);
  mcp2515.setNormalMode();
  mcp2515.sendMessage(&configurationMsg);
  delay(1000);

  
  //set baudrate back to keypad baudrate and continue
  mcp2515.setConfigMode();
  mcp2515.setBitrate(mcpBaudrate);
  mcp2515.setNormalMode();
   
  

  //send message to enable any CANopen keypad on the bus
  configurationMsg.can_id  = 0x000;
  configurationMsg.can_dlc = 8;
  configurationMsg.data[0] = 0x01;
  configurationMsg.data[1] = 0x00;
  configurationMsg.data[2] = 0x00;
  configurationMsg.data[3] = 0x00;
  configurationMsg.data[4] = 0x00;
  configurationMsg.data[5] = 0x00;
  configurationMsg.data[6] = 0x00;
  configurationMsg.data[7] = 0x00;
  mcp2515.sendMessage(&configurationMsg);
  delay(1000);

  //Set keypad to be already active at startup
  configurationMsg.can_id  = 0x600 + KEYPAD_CAN_ID;
  configurationMsg.can_dlc = 8;
  configurationMsg.data[0] = 0x2F;
  configurationMsg.data[1] = 0x12;
  configurationMsg.data[2] = 0x20;
  configurationMsg.data[3] = 0x00;
  configurationMsg.data[4] = 0x01;
  configurationMsg.data[5] = 0x00;
  configurationMsg.data[6] = 0x00;
  configurationMsg.data[7] = 0x00;
  mcp2515.sendMessage(&configurationMsg);
  delay(1000);

  //Set startup LED show
  configurationMsg.can_id  = 0x600 + KEYPAD_CAN_ID;
  configurationMsg.can_dlc = 8;
  configurationMsg.data[0] = 0x2F;
  configurationMsg.data[1] = 0x14;
  configurationMsg.data[2] = 0x20;
  configurationMsg.data[3] = 0x00;
  configurationMsg.data[4] = 0x01;
  configurationMsg.data[5] = 0x00;
  configurationMsg.data[6] = 0x00;
  configurationMsg.data[7] = 0x00;
  mcp2515.sendMessage(&configurationMsg);
  delay(1000);

  //Set periodic transmission time
  configurationMsg.can_id  = 0x600 + KEYPAD_CAN_ID;
  configurationMsg.can_dlc = 8;
  configurationMsg.data[0] = 0x2B;
  configurationMsg.data[1] = 0x00;
  configurationMsg.data[2] = 0x18;
  configurationMsg.data[3] = 0x05;
  configurationMsg.data[4] = 0x28;
  configurationMsg.data[5] = 0x01;
  configurationMsg.data[6] = 0x00;
  configurationMsg.data[7] = 0x00;
  mcp2515.sendMessage(&configurationMsg);
  delay(1000);

  //Set keypad mode to periodic transmission
  configurationMsg.can_id  = 0x600 + KEYPAD_CAN_ID;
  configurationMsg.can_dlc = 8;
  configurationMsg.data[0] = 0x2F;
  configurationMsg.data[1] = 0x00;
  configurationMsg.data[2] = 0x18;
  configurationMsg.data[3] = 0x05;
  configurationMsg.data[4] = 0x01;
  configurationMsg.data[5] = 0x00;
  configurationMsg.data[6] = 0x00;
  configurationMsg.data[7] = 0x00;
  mcp2515.sendMessage(&configurationMsg);
  delay(1000);
}

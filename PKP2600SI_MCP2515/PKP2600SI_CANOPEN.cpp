#include "Arduino.h"
#include "PKP2600SI_CANOPEN.h"

// Interrupt Callback Functions
void CANKeypad::mcpInterruptCallback(){
  _mcp_interrupt=true;
}

void CANKeypad::timerOneInterruptCallback(){
  _periodicSendNumber++;
  _periodicSentFlag=false;
}


//*********************************
//*********************************
//********** CONSTRUCTOR **********
//*********************************
//*********************************
CANKeypad::CANKeypad(MCP2515 &mcp1,byte intPin, uint8_t CAN_ID, bool passEnable) :
      //Constructor Initializer List
      _PKP_MCP(mcp1), //Passing the MCP object to an object variable in this library to be used internally
      interruptPin(intPin),
      keypadCANID(CAN_ID),
      _passwordEnable(passEnable)
{
//constructor things
  
}

void CANKeypad::begin(CAN_SPEED CANSpeed, CAN_CLOCK mcpClockSpeed)
{
  // ------ Initializing MCP2515 -------
  //_libSerial->println("Intializing");
  bool MCP_OK = false;
  while(!MCP_OK){
    if(_PKP_MCP.reset() == MCP2515::ERROR_OK){
      _libSerial->println("MCP OK");
      MCP_OK = true;
    }
    delay(5);
  }
  setupMessages();

  // ------ CONFIGURING MCP2515 -------
  _PKP_MCP.setConfigMode();
  //here is where you set masks and filters
  //Set both masks to only pass standard frame IDs to the filters. note that if these masks are set, your filters also have to be configured to receive matching IDs, else and interrupt will never be triggered.
  _PKP_MCP.setFilterMask(MCP2515::MASK0, false, 0x7FF);
  _PKP_MCP.setFilterMask(MCP2515::MASK1, false, 0x7FF);

  //  Filters 0 and 1 apply to MASK0 and go to Receive buffer 0, Filters 2-5 apply to MASK1 and go to receive buffer 1
  //  The library takes care of the filter's length and the way it can filter data bytes to where if you are using a standard frame, your filter only needs to be the ID you are looking for. 
  //_PKP_MCP.setFilter(const RXF num, const bool ext, const uint32_t ulData)
  _PKP_MCP.setFilter(MCP2515::RXF0, false, 0x180 + keypadCANID);
  //_PKP_MCP.setFilter(MCP2515::RXF1, false, 0x7FF);
  _PKP_MCP.setFilter(MCP2515::RXF2, false, 0x180 + keypadCANID);
  //_PKP_MCP.setFilter(MCP2515::RXF3, false, 0x7FF);
  //_PKP_MCP.setFilter(MCP2515::RXF4, false, 0x7FF);
  //_PKP_MCP.setFilter(MCP2515::RXF5, false, 0x7FF);

  // ------ setting up keypad password -------
  if(!_passwordEnable){
    _keypadUnlocked=true;
  }
  else _keypadUnlocked=false;

  //Set up interrupts
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), mcpInterruptCallback, FALLING);
  Timer1.initialize(400000); //Set Timer1 to trigger interrupt every 320ms
  Timer1.attachInterrupt(timerOneInterruptCallback); 

  _PKP_MCP.setBitrate(CANSpeed, mcpClockSpeed);
  _PKP_MCP.setNormalMode();
  _PKP_MCP.sendMessage(&_startCANopenNodeMsg); //Not really needed since the keypad is set to be active on startup, but when uploading arduino code the MCP sends a bunch of 0s over CAN which stops the keypad CANopen node.
}

void CANKeypad::process()
{
   _currentMillis=millis();
   if(!_mcp_interrupt && !digitalRead(interruptPin) && (_currentMillis - _lastInterruptTime)>450){ //Periodically in case an interrupt is missed for some reason
     _mcp_interrupt=true;
   }
   if(_mcp_interrupt){
    _lastInterruptTime=_currentMillis;
    //_libSerial->println("Interrupt Received");
    _mcp_interrupt = false;
    uint8_t irq = _PKP_MCP.getInterrupts();
    if(irq & (MCP2515::CANINTF_RX0IF | MCP2515::CANINTF_RX1IF)){ //if the interrupt came from one of the receive buffers, do the following
      //_libSerial->println("Interrupt came from receive buffer");
      if (_PKP_MCP.readMessage(&rcvMsg) == MCP2515::ERROR_OK) {  
        //_libSerial->print("CAN ID: ");
        //_libSerial->println(rcvMsg.can_id);    
        if(rcvMsg.can_id == (0x180 + keypadCANID)){
          _lastKeyReceiveMillis = _currentMillis;
          if(_keypadUnlocked){
            decodeKeys(rcvMsg);
          }
          else{
            passwordHandler(rcvMsg);
          }
        }
        else{
          interruptAvailable = true;
        }
      }
    }
    if(irq & MCP2515::CANINTF_MERRF){
      //_libSerial->println("Message TX/RX Error, MERRF flag has been set");
      _PKP_MCP.MCP2515::clearMERR();
    }
    if(irq & MCP2515::CANINTF_WAKIF){
      //_libSerial->printlnprintln("Bus Activity Wake-up Interrupt");
    }
    if(irq & MCP2515::CANINTF_ERRIF){
      //_libSerial->printlnprintln("Error Interrupt Flag bit (multiple sources from EFLG Register)");
      if(_PKP_MCP.MCP2515::checkError()){
        /*
          Serial.println("Check Error Returned True");
          uint8_t erf = mcp2515.MCP2515::getErrorFlags();
          if(erf & 0x01)_libSerial->println("EWARN: Error Warning Flag bit");
          if(erf & 0x02)_libSerial->println("RXWAR: Receive Error Warning Flag bit");
          if(erf & 0x04)_libSerial->println("TXWAR: Transmit Error Warning Flag bit");
          if(erf & 0x08)_libSerial->println("RXEP: Receive Error-Passive Flag bit");
          if(erf & 0x10)_libSerial->printlnn("TXEP: Transmit Error-Passive Flag bit");
          if(erf & 0x20)_libSerial->println("TXBO: Bus-Off Error Flag bit");
          if(erf & 0x40)_libSerial->println("RX0OVR: Receive Buffer 0 Overflow Flag bit");
          if(erf & 0x80)_libSerial->println("RX1OVR: Receive Buffer 1 Overflow Flag bit");
        */
      }
    }    
  }
  periodicSend();
  checkForKeypad();
}

void CANKeypad::setupMessages()
{
  _keypadMasterStatusMsg.can_id  = 0x662;
  _keypadMasterStatusMsg.can_dlc = 8;
  _keypadMasterStatusMsg.data[0] = 0xFF;  //byte 0 always is 0xFF
  _keypadMasterStatusMsg.data[1] = (uint8_t)_keypadUnlocked; //keypad unlocked/locked status.  0x00 = Locked, 0x01 = unlocked
  _keypadMasterStatusMsg.data[2] = 0x00; //Keys 1 and 2 status (MSB is key 1, LSB is key 2)
  _keypadMasterStatusMsg.data[3] = 0x00; //Keys 3 and 4 status (MSB is key 3, LSB is key 4)
  _keypadMasterStatusMsg.data[4] = 0x00; //Keys 5 and 6 status (MSB is key 5, LSB is key 6)
  _keypadMasterStatusMsg.data[5] = 0x00; //Keys 7 and 8 status (MSB is key 7, LSB is key 8)
  _keypadMasterStatusMsg.data[6] = 0x00; //Keys 9 and 10 status (MSB is key 9, LSB is key 10)
  _keypadMasterStatusMsg.data[7] = 0x00; //Keys 11 and 12 status (MSB is key 11, LSB is key 12)

  
  _keyColorMsg.can_id  = 0x200 + keypadCANID;
  _keyColorMsg.can_dlc = 8;
  _keyColorMsg.data[0] = 0xFF;
  _keyColorMsg.data[1] = 0x0F;
  _keyColorMsg.data[2] = 0x00;
  _keyColorMsg.data[3] = 0x00;
  _keyColorMsg.data[4] = 0x00;
  _keyColorMsg.data[5] = 0x00;
  _keyColorMsg.data[6] = 0x00;
  _keyColorMsg.data[7] = 0x00;

  _backlightBrightnessMsg.can_id  = 0x500 + keypadCANID;
  _backlightBrightnessMsg.can_dlc = 8;
  _backlightBrightnessMsg.data[0] = (uint8_t)(0x3F*(double)_backlightBrightness/(double)100);
  _backlightBrightnessMsg.data[1] = _defaultBacklightColor;
  _backlightBrightnessMsg.data[2] = 0x00;
  _backlightBrightnessMsg.data[3] = 0x00;
  _backlightBrightnessMsg.data[4] = 0x00;
  _backlightBrightnessMsg.data[5] = 0x00;
  _backlightBrightnessMsg.data[6] = 0x00;
  _backlightBrightnessMsg.data[7] = 0x00;

  _keyBrightnessMsg.can_id  = 0x400 + keypadCANID;
  _keyBrightnessMsg.can_dlc = 8;
  _keyBrightnessMsg.data[1] = 0x00;
  _keyBrightnessMsg.data[2] = 0x00;
  _keyBrightnessMsg.data[3] = 0x00;
  _keyBrightnessMsg.data[4] = 0x00;
  _keyBrightnessMsg.data[5] = 0x00;
  _keyBrightnessMsg.data[6] = 0x00;
  _keyBrightnessMsg.data[7] = 0x00;

  _keyBlinkMsg.can_id  = 0x300 + keypadCANID;
  _keyBlinkMsg.can_dlc = 8;
  _keyBlinkMsg.data[1] = 0x00;
  _keyBlinkMsg.data[2] = 0x00;
  _keyBlinkMsg.data[3] = 0x00;
  _keyBlinkMsg.data[4] = 0x00;
  _keyBlinkMsg.data[5] = 0x00;
  _keyBlinkMsg.data[6] = 0x00;
  _keyBlinkMsg.data[7] = 0x00;

  _startCANopenNodeMsg.can_id  = 0x000;
  _startCANopenNodeMsg.can_dlc = 8;
  _startCANopenNodeMsg.data[0] = 0x01;
  _startCANopenNodeMsg.data[1] = keypadCANID;
  _startCANopenNodeMsg.data[2] = 0x00;
  _startCANopenNodeMsg.data[3] = 0x00;
  _startCANopenNodeMsg.data[4] = 0x00;
  _startCANopenNodeMsg.data[5] = 0x00;
  _startCANopenNodeMsg.data[6] = 0x00;
  _startCANopenNodeMsg.data[7] = 0x00;

}

void CANKeypad::decodeKeys(struct can_frame canMsg){
  for(int i=0; i <12; i++){
        if(i<8){
          _buttonPresses[i]=(canMsg.data[0]>>i) & 0x1; //bit shifting the button bit to the first place, then bitwise AND operation to make the output either 0b0 or 0b1
        }
        else{
          _buttonPresses[i]=(canMsg.data[1]>>(i-8)) & 0x1;
        }
      }
      for(int i=0; i <12; i++){
        if(_lastButtonPresses[i] != _buttonPresses[i]){
          if(_buttonMode[i]==BUTTON_MODE_MOMENTARY)
            buttonState[i]=_buttonPresses[i];
          else if(_buttonMode[i]==BUTTON_MODE_TOGGLE && _buttonPresses[i]==true){
            buttonState[i]= !buttonState[i];
          }
          else if(_buttonMode[i]==BUTTON_MODE_CYCLE3 && _buttonPresses[i]==true){
            if(buttonState[i] <2) buttonState[i]++;
            else buttonState[i]=0;
          }
          keypadWriteColor();
          keypadWriteBlink();
          sendKeysStatus();
        }
      }
      for(int i=0; i <12; i++){
        _lastButtonPresses[i]=_buttonPresses[i];
      }
}

void CANKeypad::passwordHandler(struct can_frame canMsg){
    //convert can frames into button press array
    for(int i=0; i <12; i++){
      if(i<8){
        _buttonPresses[i]=(canMsg.data[0]>>i) & 0x1; //bit shifting the button bit to the first place, then bitwise AND operation to make the output either 0b0 or 0b1
      }
      else{
        _buttonPresses[i]=(canMsg.data[1]>>(i-8)) & 0x1;
      }
    }
    //do stuff with button press info
    for(int i=0; i <12; i++){
      if(_lastButtonPresses[i] != _buttonPresses[i] && _buttonPresses[i]==1){
        //check the order of buttons pressed and see if it matches the password
        _currentPass[0] = _currentPass[1];
        _currentPass[1] = _currentPass[2];
        _currentPass[2] = _currentPass[3];
        _currentPass[3] = i+1;
      }
    }
    for(int i=0; i <12; i++){
      _lastButtonPresses[i]=_buttonPresses[i];
    }
    //Check if passwords match
    bool match=true;
    for(int i=0; i <4; i++){
      if(_currentPass[i]==_keyPass[i]){
        match = match & B1;
      }
      else match = 0;
    }
    
    if(match==true && _lastMatch==false){
      //flash whole keypad green momentarily
      _startTime = _currentMillis;
      _keyColorMsg.data[0] = 0x00;
      _keyColorMsg.data[1] = 0xF0;
      _keyColorMsg.data[2] = 0xFF;
      _keyColorMsg.data[3] = 0x00;
      _keyColorMsg.data[4] = 0x00;
      _PKP_MCP.sendMessage(&_keyColorMsg);      
    }
    else if(match==true && (_currentMillis-_startTime)>800){
      //unlock keypad mode and turn off all key colors
      _keypadUnlocked=true;
      //reset password buffer in case the program comes back to password state again before a hard reset.
      for(int i=0;i<4;i++){
        _currentPass[i]=0;
      }
        //set button states to their defaults and write the respective colors
        for(int i=0;i<12;i++){
          buttonState[i]=_defaultButtonState[i];
        }
        keypadWriteColor();
    }      
    else if(_currentMillis-_redSetMillis>850 && match==false){
      //set keys colors to red periodically
        _redSetMillis=_currentMillis;
        _keyColorMsg.data[0] = 0xFF;
        _keyColorMsg.data[1] = 0x0F;
        _keyColorMsg.data[2] = 0x00;
        _keyColorMsg.data[3] = 0x00;
        _keyColorMsg.data[4] = 0x00;  
        //mcp2515.sendMessage(&keyColorMsg);    
    }
    _lastMatch=match;
}

void CANKeypad::keypadWriteColor(){ 
  _keyColorMsg.data[0] = 0x00;
  _keyColorMsg.data[1] = 0x00;
  _keyColorMsg.data[2] = 0x00;
  _keyColorMsg.data[3] = 0x00;
  _keyColorMsg.data[4] = 0x00;

  for(int i=0;i<12;i++){
    for(int j=0;j<3;j++){
      _currentColors[i][j]=false;
    }
  }

  //setting up current colors array
  for(int m=0; m<4;m++){
    for(int k=0; k<12;k++){
      if(buttonState[k]==m){
        _currentColors[k][0]=(_keysColor[m][k]>>2) & 0b1; //RED bit
        _currentColors[k][1]=(_keysColor[m][k]>>1) & 0b1; //GREEN bit
        _currentColors[k][2]=(_keysColor[m][k]) & 0b1; //BLUE bit
      }
    }
  }

  //Reorganizing colors array into proper byte format for the keypad can message
  //BYTE 0 (R8 R7 R6 R5 - R4 R3 R2 R1)
  for(int i=8;i>0;i--){
    _keyColorMsg.data[0] = _keyColorMsg.data[0] | _currentColors[i-1][0]<<(i-1);
  }
  //BYTE 1 (G4 G3 G2 G1 – R12 R11 R10 R9)
  for(int i=4;i>0;i--){
    //green part
    _keyColorMsg.data[1] = _keyColorMsg.data[1] | _currentColors[i-1][1]<<(i-1+4);
  }
  for(int i=12;i>8;i--){
    //red part
    _keyColorMsg.data[1] = _keyColorMsg.data[1] | _currentColors[i-1][0]<<(i-1-8);
  }
  //BYTE 2 (G12 G11 G10 G9 – G8 G7 G6 G5)
  for(int i=12;i>4;i--){
    _keyColorMsg.data[2] = _keyColorMsg.data[2] | _currentColors[i-1][1]<<(i-1-4);
  }
  //BYTE 3 (B8 B7 B6 B5 – B4 B3 B2 B1)
  for(int i=8;i>0;i--){
    _keyColorMsg.data[3] = _keyColorMsg.data[3] | _currentColors[i-1][2]<<(i-1);
  }
  //BYTE 4 (00 00 00 00 00 – B12 B11 B10 B9)
  for(int i=12;i>8;i--){
    _keyColorMsg.data[4] = _keyColorMsg.data[4] | _currentColors[i-1][2]<<(i-1-8);
  }
  _PKP_MCP.sendMessage(&_keyColorMsg);
}

void CANKeypad::keypadWriteBlink(){
  
  _keyBlinkMsg.data[0] = 0x00;
  _keyBlinkMsg.data[1] = 0x00;
  _keyBlinkMsg.data[2] = 0x00;
  _keyBlinkMsg.data[3] = 0x00;
  _keyBlinkMsg.data[4] = 0x00;

  for(int i=0;i<12;i++){
    for(int j=0;j<3;j++){
      _currentBlinks[i][j]=false;
    }
  }

  //setting up current Blink colors array
  for(int m=0; m<4;m++){
    for(int k=0; k<12;k++){
      if(buttonState[k]==m){
        _currentBlinks[k][0]=(_keysBlinkColor[m][k]>>2) & 0b1; //RED bit
        _currentBlinks[k][1]=(_keysBlinkColor[m][k]>>1) & 0b1; //GREEN bit
        _currentBlinks[k][2]=(_keysBlinkColor[m][k]) & 0b1; //BLUE bit
      }
    }
  }

  //Reorganizing colors array into proper byte format for the keypad can message
  //BYTE 0 (R8 R7 R6 R5 - R4 R3 R2 R1)
  for(int i=8;i>0;i--){
    _keyBlinkMsg.data[0] = _keyBlinkMsg.data[0] | _currentBlinks[i-1][0]<<(i-1);
  }
  //BYTE 1 (G4 G3 G2 G1 – R12 R11 R10 R9)
  for(int i=4;i>0;i--){
    //green part
    _keyBlinkMsg.data[1] = _keyBlinkMsg.data[1] | _currentBlinks[i-1][1]<<(i-1+4);
  }
  for(int i=12;i>8;i--){
    //red part
    _keyBlinkMsg.data[1] = _keyBlinkMsg.data[1] | _currentBlinks[i-1][0]<<(i-1-8);
  }
  //BYTE 2 (G12 G11 G10 G9 – G8 G7 G6 G5)
  for(int i=12;i>4;i--){
    _keyBlinkMsg.data[2] = _keyBlinkMsg.data[2] | _currentBlinks[i-1][1]<<(i-1-4);
  }
  //BYTE 3 (B8 B7 B6 B5 – B4 B3 B2 B1)
  for(int i=8;i>0;i--){
    _keyBlinkMsg.data[3] = _keyBlinkMsg.data[3] | _currentBlinks[i-1][2]<<(i-1);
  }
  //BYTE 4 (00 00 00 00 00 – B12 B11 B10 B9)
  for(int i=12;i>8;i--){
    _keyBlinkMsg.data[4] = _keyBlinkMsg.data[4] | _currentBlinks[i-1][2]<<(i-1-8);
  }
  _PKP_MCP.sendMessage(&_keyBlinkMsg);
}

void CANKeypad::sendKeysStatus(){
  _keypadMasterStatusMsg.data[1] = (uint8_t)_keypadUnlocked; //keypad unlocked/locked status.  0x00 = Locked, 0x01 = unlocked
  _keypadMasterStatusMsg.data[2] = buttonState[0]<<4 | buttonState[1]; //Keys 1 and 2 status (MSB is key 1, LSB is key 2)
  _keypadMasterStatusMsg.data[3] = buttonState[2]<<4 | buttonState[3]; //Keys 3 and 4 status (MSB is key 3, LSB is key 4)
  _keypadMasterStatusMsg.data[4] = buttonState[4]<<4 | buttonState[5]; //Keys 5 and 6 status (MSB is key 5, LSB is key 6)
  _keypadMasterStatusMsg.data[5] = buttonState[6]<<4 | buttonState[7]; //Keys 7 and 8 status (MSB is key 7, LSB is key 8)
  _keypadMasterStatusMsg.data[6] = buttonState[8]<<4 | buttonState[9]; //Keys 9 and 10 status (MSB is key 9, LSB is key 10)
  _keypadMasterStatusMsg.data[7] = buttonState[10]<<4 | buttonState[11]; //Keys 11 and 12 status (MSB is key 11, LSB is key 12)
  _PKP_MCP.sendMessage(&_keypadMasterStatusMsg);
}

void CANKeypad::setKeypadPassword(uint8_t password[4]){
  for(int i=0;i<4;i++){
    _keyPass[i] = password[i];
  }
}

void CANKeypad::setKeyBrightness(uint8_t brightness){
  if(brightness>100)brightness=100;
  if(brightness<0)brightness=0;
  
  _keyBrightnessMsg.data[0] = 0x3F*brightness/100;
  _PKP_MCP.sendMessage(&_keyBrightnessMsg);
}

void CANKeypad::setBacklightBrightness(uint8_t color, uint8_t brightness){
  if(brightness>100)brightness=100;
  if(brightness<0)brightness=0;

  _backlightBrightnessMsg.data[0] = 0x3F*brightness/100;
  _backlightBrightnessMsg.data[1] = color; //this color set part only works on SW REV1.5 (won't do anything on 1.4 or below)
  _PKP_MCP.sendMessage(&_backlightBrightnessMsg);
}

void CANKeypad::setKeyColor(uint8_t keyNumber, uint8_t colors[4], uint8_t blinkColors[4]){
  for(int i=0;i<4;i++){
    _keysColor[i][keyNumber] = colors[i];
    _keysBlinkColor[i][keyNumber] = blinkColors[i];
  }
}

void CANKeypad::setKeyMode(uint8_t keyNumber, uint8_t keyMode){
  _buttonMode[keyNumber]=keyMode;
}

void CANKeypad::setSerial(Stream *serialReference)
{
  _libSerial=serialReference;
}

void CANKeypad::periodicSend(){
  if(_periodicSentFlag==false){
    //_libSerial->println("Periodic Sending");
    if(_periodicSendNumber>4)_periodicSendNumber=0;
  
    //Do periodic sending of messages
    switch (_periodicSendNumber) {
      case 0:
        // Send Backlight brightness/color Message
        _PKP_MCP.sendMessage(&_backlightBrightnessMsg);
        break;
      case 1:
        // Send Key Color Message
        if(_keypadUnlocked==true){
          keypadWriteColor();
        }
        else{
          _PKP_MCP.sendMessage(&_keyColorMsg);
        }
        break;
      case 2:
      //Send key brightness Message
        _PKP_MCP.sendMessage(&_keyBrightnessMsg);
        break;
      case 3:   
        // Send Key Blink Message
        if(_keypadUnlocked==true){
          keypadWriteBlink();
        }
        break;
      case 4:   
        // Send Keypad Status Summary Message
        sendKeysStatus();
        break;
      default:
        break;
    }
    _periodicSentFlag=true;
  }
}

void CANKeypad::checkForKeypad(){
  if(_currentMillis-_lastKeyReceiveMillis>2000 && _currentMillis-_lastKeyReceiveMillis<2500){
    //uncomment this line if you want the keypad to be locked when it detects an unplug
    //keypadUnlocked=false;

    //set all button states back to default button states as a safety feature
    for(int i=0;i<12;i++){
      buttonState[i]=_defaultButtonState[i];
    }
  }
}

void CANKeypad::setDefaultButtonStates(uint8_t defaultStates[12])
{
  for(int i=0;i<12;i++){
    _defaultButtonState[i]=defaultStates[i];
    buttonState[i]=defaultStates[i];
  }
}

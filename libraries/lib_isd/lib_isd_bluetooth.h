#ifndef _LIB_ISD_BLUETOOTH_H__
#define _LIB_ISD_BLUETOOTH_H__

#include <SoftwareSerial.h>

namespace libISD {


class ISDControllerBleCallback {
public:
    ISDControllerBleCallback(){}
    virtual ~ISDControllerBleCallback(){}

    virtual void onUpdateAudioVolume( int val ) = 0;
    virtual void onPlayImperialMarch() = 0;
    virtual void onPlayVadersIntro() = 0;
    virtual void onGarbageChuteOn( bool val ) = 0;
    virtual void onPowerUpLightSpeedEngines( bool val, bool withAudio ) = 0;
    virtual void onPowerUpSubLightEngines( bool val, bool withAudio) = 0;
    virtual void onPowerUpMainSystems( bool val, bool withAudio) = 0;
    virtual void onStartPowerSequence(bool withAudio) = 0;
    virtual void onEnginesStartSequence(bool withAudio) = 0;
    virtual void onStopAudio() = 0;
};




class BLEDevice {
  public:

  class ReadGuard {
  public:
    ReadGuard(bool& v, bool initVal) : okToRead_(v) {
        okToRead_ = initVal;
    }
    ~ReadGuard() {
      okToRead_ = !okToRead_;
    }
  private:
      bool& okToRead_;
  };
  
  BLEDevice( int icTX, int icRX, int analogState, int ledOut, bool enableSerialOut=true):
     m_enableSerialOut(enableSerialOut), 
     m_serialDevice(icTX,icRX),
     callbackPtr(NULL),
     analogStatePin(analogState),
     ledOutPin(ledOut),
     stateLedOn(false),
      stateCmdRcvd(false){
      
    lastTime = 0;
    stateLastTime = 0;
    currentCmdIdx = 0;
    memset(currentCmd,0,sizeof(currentCmd));
    okToRead = false;
  }

  enum Constants {
    STX = 0x01,
    ETX = 0x03,
    MSG_UPDATE_AUDIO_VOLUME = 0xFF,
    MSG_PLAY_IMPERIAL_MARCH = 0xFE,
    MSG_GARBAGE_CHUTE_ON = 0xFD,
    MSG_POWER_UP_LIGHT_SPEED_ENGINES = 0xFC,
    MSG_POWER_UP_SUBLIGHT_ENGINES = 0xFB,
    MSG_POWER_UP_MAIN_SYSTEMS = 0xFA,
    MSG_START_POWER_SEQUENCE = 0xF8,
    MSG_PLAY_VADER_INTRO = 0xF7,
    MSG_ENGINES_START_SEQUENCE = 0xF6,
    MSG_STOP_AUDIO = 0xF5,

    ARG_WITH_AUDIO = 0x02, //value in second bit of 8 bits
    ARG_WITH_BOOL_MASK = 0xFF - 0xFE,
    ARG_WITH_AUDIO_MASK = 0xFF-0xFD,

    READ_TIMEOUT = 2000,
    STATE_TIMEOUT = 1000,
    HM_10_MIN_VAL = 300 
  };
  
  static inline bool booleanArg(byte arg) {
      return (arg & ARG_WITH_BOOL_MASK) ? true : false;
  }

  static inline bool audioArg(byte arg) {
      return ((arg & ARG_WITH_AUDIO_MASK)>>1) ? true : false;
  }
  
  
  void begin( ISDControllerBleCallback* callback, int baudRate=9600 ) {
    callbackPtr = callback;
    
    if (m_enableSerialOut) {
        /*
      Serial.begin(115200);
      char tmp[256];
      snprintf(tmp,sizeof(tmp)-1,"Started serial out at %d baud", 115200 );
      Serial.println(tmp);
      
      */
    }
    
    delay(1000);

    m_serialDevice.begin(baudRate);
    delay(1000);
    okToRead = true;

    if (ledOutPin > 1 ) {
      pinMode(ledOutPin,OUTPUT);
      digitalWrite(ledOutPin, LOW);
    }
    stateLastTime = millis();
    //Serial.println("ble ---b");
  }

  void checkState() {
    
    if ((analogStatePin < 0) || (ledOutPin < 2)) {
      return;
    }

    if (stateCmdRcvd && ledOutPin > 1) {
        unsigned long cur = millis();
        int delta = cur - stateLastTime;
        int pinVal = digitalRead(ledOutPin);
        if (delta < 10 && pinVal == LOW) {
            digitalWrite(ledOutPin, HIGH);
            //Serial.println("....");
        }
        else if ((delta > 250 && delta < 300) && pinVal == HIGH) {
            digitalWrite(ledOutPin, LOW);
            //Serial.println(".");
        }
        else if ((delta > 300 && delta < 310) && pinVal == LOW) {
            digitalWrite(ledOutPin, HIGH);
            //Serial.println("....");
        }
        else if (delta > 550 && delta < 600 && pinVal == HIGH) {
            digitalWrite(ledOutPin, LOW);
            stateLastTime = millis();
            stateCmdRcvd = false;
            //Serial.println(".");
            //Serial.println("-");
        }
    }
    else {
        int stateVal = analogRead(analogStatePin);
        if ((millis() - stateLastTime) > STATE_TIMEOUT) {
            stateLedOn = !stateLedOn;

            digitalWrite(ledOutPin, (stateLedOn || (stateVal > HM_10_MIN_VAL)) ? HIGH : LOW);

            stateLastTime = millis();
        }
    }
  }
  
  void loop() {
    lastTime = millis();
    
    checkState();
    
    if (!okToRead) {
       
      return;
    }
    
    
    delay(1);
    int avail = m_serialDevice.available();    
    //delay(1);
    while (avail > 0) {
      uint8_t  in_ch = m_serialDevice.read();
      
      
      checkState();
      
      if ((millis()-lastTime) > READ_TIMEOUT ){ // 2 second timeout        
        lastTime = millis();
        currentCmdIdx = 0;      
        
        return;
      }
               
      currentCmd[currentCmdIdx] = (byte)in_ch;
      
      currentCmdIdx++;
  
      if (currentCmdIdx == sizeof(currentCmd)) {
          
        processCmd();
        
        currentCmdIdx = 0;      
      }
      delay(2);
    }
  }

  void processCmd() {
    if (currentCmd[0] != STX || currentCmd[3] != ETX) {
        DPRINTLN("Bad command frame, no STX or ETX");        
      return;
    }

    byte cmd = currentCmd[1];
    byte arg = currentCmd[2];

    //Serial.println(cmd);

    stateCmdRcvd = true;
    digitalWrite(ledOutPin, LOW);
    stateLastTime = millis();
    checkState();

    

    switch (cmd) {
      case MSG_UPDATE_AUDIO_VOLUME : {
        if (callbackPtr){
          callbackPtr->onUpdateAudioVolume((int)arg);
        }
      }
      break;

      case MSG_PLAY_IMPERIAL_MARCH : {
        if (callbackPtr){
          callbackPtr->onPlayImperialMarch();
        }
      }
      break;

      case MSG_PLAY_VADER_INTRO: {
          if (callbackPtr) {
              callbackPtr->onPlayVadersIntro();
          }
      }
      break;

      case MSG_GARBAGE_CHUTE_ON : {
        if (callbackPtr){
          callbackPtr->onGarbageChuteOn(arg?true:false);
        }
      }
      break;

      case MSG_POWER_UP_LIGHT_SPEED_ENGINES : {
        if (callbackPtr){
          callbackPtr->onPowerUpLightSpeedEngines(booleanArg(arg), audioArg(arg));
        }
      }
      break;

      case MSG_POWER_UP_SUBLIGHT_ENGINES : {
        if (callbackPtr){
          callbackPtr->onPowerUpSubLightEngines(booleanArg(arg), audioArg(arg));
        }
      }
      break;

      case MSG_POWER_UP_MAIN_SYSTEMS : {
        if (callbackPtr){
          callbackPtr->onPowerUpMainSystems(booleanArg(arg), audioArg(arg));
        }
      }
      break;

      case MSG_START_POWER_SEQUENCE: {
          if (callbackPtr) {
              callbackPtr->onStartPowerSequence(audioArg(arg));
          }
      }
      break;

      case MSG_ENGINES_START_SEQUENCE: {
          if (callbackPtr) {
              callbackPtr->onEnginesStartSequence(audioArg(arg));
          }
      }
      break;

      case MSG_STOP_AUDIO: {
          if (callbackPtr) {
              callbackPtr->onStopAudio();
          }
      }
      break;
      
      
      
      default : {
          DPRINTLN("Unknown command!");
          //Serial.println("!U");
         return;
      }
      break;
    }
  }
  
  void command(String cmdStr, String descStr, String* strPtr) {
      ReadGuard rg(okToRead, false);

      //Serial.println(cmdStr.c_str());


      unsigned long t1 = millis();
      m_serialDevice.println(cmdStr);
      while (true) {
          char in_char = m_serialDevice.read();
          //Serial.println(in_char);
          if (int(in_char) == -1 or int(in_char) == 42) {
              if ((millis() - t1) > 2000) { // 2 second timeout
                  Serial.println("Err");
                  return "Err";
              }
              continue;
          }
          if (in_char == '\n') {
              if (m_enableSerialOut) {
                  //Serial.print("Bluetooth ");
                  //Serial.print(descStr);
                  if (strPtr != NULL) {
                      Serial.println(*strPtr);// .substring(0, tmpStr.length()));
                  }
              }

              if (m_serialDevice.available()) {

                  continue;
              }
              else {
                  return;
              }
          }
          if (in_char >= 0x20 && in_char <= 127) {
              if (strPtr != NULL) {
                  (*strPtr) += in_char;
              }
          }
      }
  }

  void command(String cmdStr, String descStr) {
      command(cmdStr, descStr, NULL);
  }

  void command(String cmdStr, String descStr, String& result){
      command(cmdStr, descStr, &result);
  }

  void getDeviceName(String& result) {
    command("AT+NAME","Device Name: ", result);
  }
  

  void rename() {      
      command("AT+NAMEISD Obliterator","rename ");
  }


  void help(String& result) {    
    ReadGuard rg(okToRead,false);
    
    int crLfCount = 0;

    unsigned long t1 = millis();
    m_serialDevice.println("AT+HELP");

    while (true) {
      char in_ch = m_serialDevice.read();
      if ((millis()-t1)>2000){ // 2 second timeout
        //Serial.println("ble-to");
        break;
      }
      
      if ( int(in_ch) == -1 or int(in_ch)==42) {
        continue;
      }

      result += in_ch;
      
      
      bool breakLoop = false;
      if (in_ch == '\n'){
          
        if (result.length() == 2) { 
            if (result[0] == '\r' && result[1] == '\n') {
                
              if (crLfCount == 0) {
                crLfCount = 1;
                continue;
              }
              breakLoop = true;
            }
        }

        if (breakLoop) {
            break;
        }
        delay(100);
        
      }    
    }

    if (m_enableSerialOut) {
        Serial.println(result);
    }
  }
  

private:
  
    //String tmpStr;
  bool m_enableSerialOut;
  SoftwareSerial m_serialDevice;
  unsigned long lastTime;
  unsigned long stateLastTime;
  byte currentCmd[4];
  byte currentCmdIdx;
  ISDControllerBleCallback* callbackPtr;
  bool okToRead;
  int analogStatePin;
  int ledOutPin;
  bool stateLedOn;
  bool stateCmdRcvd;
};


  
}



#endif //_LIB_ISD_BLUETOOTH_H__


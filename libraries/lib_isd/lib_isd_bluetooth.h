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
    virtual void onGarbageChuteOn( bool val ) = 0;
    virtual void onPowerUpLightSpeedEngines( bool val ) = 0;
    virtual void onPowerUpSubLightEngines( bool val ) = 0;
    virtual void onPowerUpMainSystems( bool val ) = 0;
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

    READ_TIMEOUT = 2000,
    STATE_TIMEOUT = 1000,
    HM_10_MIN_VAL = 300 
  };
  
  
  
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
            Serial.println("....");
        }
        else if ((delta > 250 && delta < 300) && pinVal == HIGH) {
            digitalWrite(ledOutPin, LOW);
            Serial.println(".");
        }
        else if ((delta > 300 && delta < 310) && pinVal == LOW) {
            digitalWrite(ledOutPin, HIGH);
            Serial.println("....");
        }
        else if (delta > 550 && delta < 600 && pinVal == HIGH) {
            digitalWrite(ledOutPin, LOW);
            stateLastTime = millis();
            stateCmdRcvd = false;
            Serial.println(".");
            Serial.println("-");
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
        //Serial.println("!");
      return;
    }
    
    int avail = m_serialDevice.available();
    delay(1);
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

    Serial.println(cmd);
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

      case MSG_GARBAGE_CHUTE_ON : {
        if (callbackPtr){
          callbackPtr->onGarbageChuteOn(arg?true:false);
        }
      }
      break;

      case MSG_POWER_UP_LIGHT_SPEED_ENGINES : {
        if (callbackPtr){
          callbackPtr->onPowerUpLightSpeedEngines(arg?true:false);
        }
      }
      break;

      case MSG_POWER_UP_SUBLIGHT_ENGINES : {
        if (callbackPtr){
          callbackPtr->onPowerUpSubLightEngines(arg?true:false);
        }
      }
      break;

      case MSG_POWER_UP_MAIN_SYSTEMS : {
        if (callbackPtr){
          callbackPtr->onPowerUpMainSystems(arg?true:false);
        }
      }
      break;
      
      default : {
          DPRINTLN("Unknown command!");
         return;
      }
      break;
    }
  }
  
  
  String command(String cmdStr,String descStr){
    ReadGuard rg(okToRead,false);
    
    Serial.println(cmdStr);
    
    tmpStr = "";
    unsigned long t1 = millis();
    m_serialDevice.println(cmdStr);
    while (true){
      char in_char = m_serialDevice.read();
      //Serial.println(in_char);
      if (int(in_char)==-1 or int(in_char)==42){
        if ((millis()-t1)>2000){ // 2 second timeout
          Serial.println("Err");
          return "Err";
        }
        continue;
      }
      if (in_char=='\n'){
        if (m_enableSerialOut) {
          Serial.print("Bluetooth "+descStr);
          Serial.println(tmpStr);// .substring(0, tmpStr.length()));
        }

        if (m_serialDevice.available()) {
          
          continue;
        }
        else {
          return tmpStr;
        }
      }
      if (in_char >= 0x20 && in_char <= 127) {
          tmpStr += in_char;
      }
    }

    return tmpStr;
  }

  String getDeviceName() {
    return  command("AT+NAME","Device Name: " );
  }
  

  void rename() {
      command("AT+NAMEISD Obliterator","rename ");

  }


  void help() {    
    ReadGuard rg(okToRead,false);
    int crLfCount = 0;
    tmpStr = "";
    unsigned long t1 = millis();
    m_serialDevice.println("AT+HELP");
    while (true) {
      char in_ch = m_serialDevice.read();
      if ((millis()-t1)>2000){ // 2 second timeout
        Serial.println("timed out");
        break;
      }
      if ( int(in_ch) == -1 or int(in_ch)==42) {
        continue;
      }
      tmpStr += in_ch;
      if (in_ch == '\n'){
        if (tmpStr==String('\r')+String('\n')) {
          if (crLfCount == 0) {
            crLfCount = 1;          
            continue;
          }
          break;
        }
        if (m_enableSerialOut) {
          Serial.print(tmpStr);
        }
        tmpStr = "";
      }
    }

    Serial.println("done with help");
  }
  

private:
  String tmpStr;
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


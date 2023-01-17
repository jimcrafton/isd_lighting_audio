#ifndef _LIB_ISD_I2C_H__
#define _LIB_ISD_I2C_H__

#include <Wire.h>

namespace libISD {

class ISDControllerI2CCallback {
public:
    ISDControllerI2CCallback(){}
    virtual ~ISDControllerI2CCallback(){}

    virtual void onUpdateAudioVolume( int val ) = 0;
    virtual void onPlay(int song) = 0;
    virtual void onServerInit() = 0;
};


namespace i2c {
    enum Constants {
        I2C_BUS_ID = 5,
        STX = 0x01,
        ETX = 0x03,
        MSG_UPDATE_AUDIO_VOLUME = 0xFF,
        MSG_PLAY = 0xFE,
        MSG_INIT_FROM_SERVER = 0xFD,
    };
}

class  ServerI2C{
  public:

  void init() {
     Wire.begin();
  }

  void sendCommand(byte cmd, byte arg) {
    Wire.beginTransmission(i2c::I2C_BUS_ID);
    Wire.write(i2c::STX);
    Wire.write(cmd);
    Wire.write(arg);
    Wire.write(i2c::ETX);
    Wire.endTransmission();
    delay(100);
  }
  
};


class  ClientI2C{
  public:

  static ClientI2C* instance;

  ClientI2C() {
    ClientI2C::instance = this;
    currentCmdIdx = 0;
    callbackPtr = NULL;
    
  }

  ~ClientI2C() {
    ClientI2C::instance = NULL;
  }
  
  void init(ISDControllerI2CCallback* callback) {
    callbackPtr = callback;
    Wire.begin(i2c::I2C_BUS_ID);
    Wire.onReceive(ClientI2C::handleRcvEvent);
  }
  
  byte currentCmd[4];
  byte currentCmdIdx;
  ISDControllerI2CCallback* callbackPtr;

  


  void reciveEvent(int howMany) {
      

    while (Wire.available() > 0) { 
      char c = Wire.read(); 
      currentCmd[currentCmdIdx] = (byte)c;
      currentCmdIdx ++;
      if (currentCmdIdx == sizeof(currentCmd)) {
        currentCmdIdx = 0;
        processCmd();
      }
    }
    /*
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    */
  }

  void processCmd() {
    if (currentCmd[0] != i2c::STX) {
      return;
    }

    if (currentCmd[3] != i2c::ETX) {
      return;
    }

    
    switch(currentCmd[1]) {
      case i2c::MSG_UPDATE_AUDIO_VOLUME : {
        if (callbackPtr) {
          callbackPtr->onUpdateAudioVolume(currentCmd[2]);
        }
      }
      break;

      case i2c::MSG_PLAY : {
        if (callbackPtr) {
          callbackPtr->onPlay(currentCmd[2]);
        }
      }
      break;
       
      case i2c::MSG_INIT_FROM_SERVER: {
          if (callbackPtr) {
              callbackPtr->onServerInit();
          }
      }
       break;
      
      default : {
        
      }
      break;
    }
  }
  
  static void handleRcvEvent(int howMany) {
    if (ClientI2C::instance) {
      ClientI2C::instance->reciveEvent(howMany);
    }
  }
};

ClientI2C* ClientI2C::instance = NULL;



}  //libISD



#endif //_LIB_ISD_I2C_H__


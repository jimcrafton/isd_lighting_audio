
//#define TESTING
//#define _DEBUG


#ifdef TESTING

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

#endif //TESTING


#include "lib_isd_model.h"

using namespace libISD;

Adafruit_NeoPixel* libISD::RgbLed::g_pixelsPtr = NULL;
Tlc5940* libISD::Led::g_tlcICPtr = NULL;
LedControllerEventDelegate* LedController::g_delegatePtr = NULL;
DFRobotDFPlayerMini* Sound::g_playerPtr = NULL;
ErrorControllerDelegate* ErrorControllerDelegate::g_delegatePtr = NULL;

ISDController isdModel;
 
void setup() {
  // put your setup code here, to run once:
Serial.begin(115200); 
//
#ifdef TESTING
 
  Serial.println("serial port initialized");
  
  #ifdef _DEBUG
    Serial.println("_DEBUG defined");
  #else
    Serial.println("!_DEBUG NOT defined!");
  #endif
  Serial.print("free mem: "); Serial.println(freeMemory());

  /*
  Serial.print("ISDSpecialSections ");Serial.println(libISD::ISDSpecialSections::sizeOf());
  Serial.print("ISDSections ");Serial.println(libISD::ISDSections::sizeOf());
  Serial.print("ISDEngines ");Serial.println(libISD::ISDEngines::sizeOf());
  Serial.print("SoundController ");Serial.println(libISD::SoundController::sizeOf());
  Serial.print("ShipSection ");Serial.println(sizeof(libISD::ShipSection));
  Serial.print("ISDErrors ");Serial.println(libISD::SoundController::sizeOf());
*/
  Serial.println(millis());
#endif //TESTING
  
  isdModel.init();

#ifdef TESTING  
  Serialr.println("inits all done");

Serial.print("time ");
Serial.println(millis());
#endif //TESTING

Serial.println("i");
  
}

void loop() {
  // put your main code here, to run repeatedly:
  isdModel.loop();
}

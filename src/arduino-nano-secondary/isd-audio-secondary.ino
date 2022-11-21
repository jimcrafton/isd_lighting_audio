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

DFRobotDFPlayerMini* Sound::g_playerPtr = NULL;
ErrorControllerDelegate* ErrorControllerDelegate::g_delegatePtr = NULL;

ISDSecondaryAudioController audioClient;

void setup() {
  // put your setup code here, to run once:
  audioClient.init();
}

void loop() {
  // put your main code here, to run repeatedly:
  audioClient.loop();
}

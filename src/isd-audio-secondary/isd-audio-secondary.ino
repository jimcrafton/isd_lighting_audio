



#include "lib_isd_model.h"
#include "lib_isd_global_inits.i"

using namespace libISD;


ISDAudioController audioClient;

void setup() {
  Serial.begin(115200); 
  // put your setup code here, to run once:
  
  audioClient.init();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  audioClient.loop();
}

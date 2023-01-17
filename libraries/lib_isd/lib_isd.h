#ifndef _LIB_ISD_H__
#define _LIB_ISD_H__


#ifdef _DEBUG
	#define DPRINTLN(str)  Serial.println(str)
	#define DPRINT(str)  Serial.print(str)
#else
#define DPRINTLN(str)
#define DPRINT(str)  
#endif



#include "lib_isd_lights_audio.h"
#include "lib_isd_bluetooth.h"
#include "lib_isd_i2c.h"

#endif //_LIB_ISD_H__



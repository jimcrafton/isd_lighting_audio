#ifndef _LIB_ISD_LIGHTS_AUDIO_H__
#define _LIB_ISD_LIGHTS_AUDIO_H__

#define TLC_5940_ENABLED  1

#define NEOPIXELS_ENABLED  1


#include <time.h> 


#if NEOPIXELS_ENABLED
  #include <Adafruit_NeoPixel.h>
  #ifdef __AVR__
   #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
  #endif
#endif //NEOPIXELS_ENABLED


#if TLC_5940_ENABLED
  #include "Tlc5940.h"
#endif //TLC_5940_ENABLED


#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"


namespace libISD {

class LedController;
class Led;
class Engine;
class ShipSection;
class SpecialSection;

class LedControllerEventDelegate {
public:
    LedControllerEventDelegate() {}
    virtual ~LedControllerEventDelegate() {}

    virtual void onLedFullOn(Led*) = 0;
    virtual void onEngineLedFullOn(Engine*) = 0;
    virtual void onSectionLedFullOn(ShipSection*) = 0;
    virtual void onSpecialSectionLedFullOn(SpecialSection*) = 0;


    virtual void onLedStarted(Led*) = 0;
    virtual void onEngineLedStarted(Engine*) = 0;
    virtual void onSectionLedStarted(ShipSection*) = 0;
    virtual void onSpecialSectionLedStarted(SpecialSection*) = 0;
};

class BaseController {
protected:
    unsigned short startTime;
public:
    enum States {
        MODIFIED = 0x01,
        ENABLED = 0x02,
        IDX_MASK = 0xFC,
        STATE_MASK = 0xFF - IDX_MASK,
        IDX_BITS = 6,
    };
    //byte idx;
    byte state;
    

    enum TimeBits {
        SECONDS_BITS = 7,
        SECONDS_MASK = 0xFFFF << SECONDS_BITS,
        MS_MASK = 0x0FFFF - SECONDS_MASK,
        INVALID_TIME = 0xFFFF,
    };

    //store as seconds from start
    //with 7 bits for seconds (0-127)
    //9 bits for milliseconds, with half resolution
    
    

    BaseController() : /*idx(0),*/ 
        state(ENABLED), 
        startTime(INVALID_TIME) {

    }

    virtual ~BaseController() {}

    void setIdx(byte i) {
        state = (STATE_MASK & state) | (i << (8 - IDX_BITS));
    }

    byte getIdx() const {
        return state >> (8 - IDX_BITS);
    }

    bool isModified() const { return (BaseController::MODIFIED & state) ? true : false; }
    void setModified(bool v) {
        if (v) {
            state |= BaseController::MODIFIED;
        }
        else {
            state &= ~BaseController::MODIFIED;
        }
    }

    bool getEnabled() const { return (BaseController::ENABLED & state) ? true : false; }

    void setEnabled(bool v) {
        if (getEnabled() != v) {
            if (v) {
                state |= BaseController::ENABLED;
            }
            else {
                state &= ~BaseController::ENABLED;
            }
            setModified(true);
        }
    }

    unsigned long getStartTime() const { 
        return (startTime >> (16 - SECONDS_BITS)) * 1000 + ((MS_MASK & startTime) * 2);
    }

    void setStartTimeInvalid() {
        startTime = INVALID_TIME;
    }

    bool isStartTimeInvalid() const { return startTime == INVALID_TIME; }

    void setStartTime(unsigned long v) {        
        startTime = (MS_MASK & ((v % 1000) / 2)) | ((v / 1000) << (16 - SECONDS_BITS));
    }

    virtual bool isEnabled(unsigned long timeFromStart) const {
        
        if (isStartTimeInvalid()) {
            return false;
        }

        unsigned long st = getStartTime();

        if (!getEnabled()) {
            return false;
        }

        return timeFromStart >= st;
    }
};

class LedController : public BaseController {
public:  
    
  static LedControllerEventDelegate* g_delegatePtr;

  LedController(): BaseController(){
    
  }

  virtual ~LedController() {
    
  }
  
  virtual void updateLed() {}
  virtual void preUpdate(unsigned long timeFromStart, byte intensityIncr) {}
  
  virtual void ledFullOn() {}
  virtual void ledStarted() {}

  virtual int getIntensity() { return -255; }

  void update() {    
    if (isModified()) {
      updateLed();
    }
    setModified(false);
  }
};



class LedCollection {
public:
	LedController** collectionPtr;
	int size;
	unsigned long startTime;
    unsigned long prevRun;
	bool enabled;
	int delayFactor;
	static char fullOnArray[50];
	static char startedArray[50];
	byte intensityIncr;
	LedCollection() :
		collectionPtr(NULL),
		size(0),
		startTime(0),
        prevRun(0),
		enabled(true),
		delayFactor(20),
		intensityIncr(2)
	{

	}

	~LedCollection() {

	}

	void setDelay(int v) {
		delayFactor = v;
	}

	byte getIntensityIncr() const { return  intensityIncr; }
	void setIntensityIncr(byte v) {
		intensityIncr = v;
	}

    void init(LedController** coll, int sz) {
	    collectionPtr = coll;
	    size = sz;
	    startTime = millis();
	    enabled = true;
    }

    bool isEnabled() const { return enabled; }
    void setEnabled(bool v) { 
    enabled = v; 
    }
  
  
    bool empty() const { return size;}

    void loop() {
	    unsigned long now = millis();
	    if (enabled == false) {
		    return;
	    }

	    //only loop every X millseconds. simulate delay()

	    unsigned long  delta = now - startTime;
	    if ((now % delayFactor == 0) || ((now-prevRun)> delayFactor)) {
		    float oldI = 0;
		    float newI = 0;
		    memset(fullOnArray, -1, sizeof(fullOnArray));
		    memset(startedArray, -1, sizeof(startedArray));

		    int events = 0;
		    for (int i = 0; i < size; i++) {

			    LedController* led = collectionPtr[i];
			    oldI = led->getIntensity();
			    if (led->isEnabled(delta)) {
				    led->preUpdate(delta, intensityIncr);
				    newI = led->getIntensity();

				    led->update();

				    if ((oldI != newI) && (newI >= 255)) {
					    fullOnArray[i] = (char)i;
					    events++;
				    }
				    else if ((oldI <= 0) && (newI > 0)) {
					    startedArray[i] = (char)i;
					    events++;
				    }
			    }
		    }

            prevRun = millis();

		    if (events == 0) {
			    return;
		    }
		    LedController* led = NULL;
		    for (int i = 0; i < size; i++) {
			    char idx = startedArray[i];
			    if (idx >= 0) {
				    led = collectionPtr[idx];
				    if (led) {
					    led->ledStarted();
				    }
			    }

			    idx = fullOnArray[i];
			    if (idx >= 0) {
				    led = collectionPtr[idx];
				    if (led) {
					    led->ledFullOn();
				    }
			    }
		    }
		    memset(fullOnArray, -1, sizeof(fullOnArray));
		    memset(startedArray, -1, sizeof(startedArray));
            
	    }
    }
};

static char LedCollection::fullOnArray[50];
static char LedCollection::startedArray[50];

#if TLC_5940_ENABLED

#define TLC5940_MAX_VAL     4095
#define TLC5940_MAX_VALF     4095.0

void writeLED(Tlc5940& tlcIC, int idx, byte value, byte intensity)
{
  //unsigned int v = min( (intensity * value)/255, 255);
  unsigned int v = min( (unsigned int)(intensity * value) / 255, 255 );
  v = map(v, 0, 255, 0, TLC5940_MAX_VAL);

  tlcIC.set(idx, v);
  
  tlcIC.update();
}
#endif //TLC_5940_ENABLED



class Led : public LedController {
public:      
  byte value;
  byte intensity;
  #if TLC_5940_ENABLED
  static Tlc5940* g_tlcICPtr;
  #endif

  Led():LedController(),value(0),intensity(0)
  
  {
    
  }

  virtual ~Led() {
    
  }
#if TLC_5940_ENABLED  
  void init(Tlc5940& ic, byte index, byte v, unsigned long st) {
      g_tlcICPtr = &ic;
    setStartTime(st);
    setIdx(index);
    setValue(v);
    setModified(true);
  }
#endif //TLC_5940_ENABLED

  virtual void ledFullOn() {
      if (g_delegatePtr) {
          g_delegatePtr->onLedFullOn(this);
      }
  }

  virtual void ledStarted() {
      if (g_delegatePtr) {
          g_delegatePtr->onLedStarted(this);
      }
  }

  void setValue(byte v) {
    if ( value != v ) {
      value = v;
      setModified(true);
    }
  }

  setIntensity(byte v) {
    if ( intensity != v ) {
      intensity = min(v,255);
      setModified(true);
    }
  }
  
  virtual int getIntensity() { return intensity; }

  void incrIntensity(byte v) {
    if (intensity < 255) {
      intensity = min(intensity + v,255);
      setModified(true);
    }
  }
  
  

  
  virtual void preUpdate(unsigned long timeFromStart, byte intensityIncr) {
    incrIntensity(intensityIncr);
  }

  void stop() {
    setIntensity(0);

    #if TLC_5940_ENABLED
    if (g_tlcICPtr) {
      writeLED(*g_tlcICPtr, this->getIdx(),0,0);

      //Serial.print(getIdx());
      //Serial.println(" >~");
    }
    #endif
    setModified(false);
  }
  
  virtual void updateLed() {
    #if TLC_5940_ENABLED
    if (g_tlcICPtr) {
      writeLED(*g_tlcICPtr, this->getIdx(),value,intensity);
      //Serial.print("+tl"); Serial.print(this->getIdx());
    }
    #endif
    
  }
};

enum BigEngines {
  LEFT_ENGINE=1,
  CENTER_ENGINE=3,
  RIGHT_ENGINE=5,
};


enum MiniEngines {
  LEFT_ENGINE_TOP=0,
  LEFT_ENGINE_BOTTOM=2,
  RIGHT_ENGINE_TOP=4,
  RIGHT_ENGINE_BOTTOM=6,
};

#if NEOPIXELS_ENABLED
void writeRgbLED(Adafruit_NeoPixel& pixels, int led, byte r, byte g, byte b, byte intensity)
{

  byte rval = (intensity * r) / 255;
  byte gval = (intensity * g) / 255;//min( (intensity * g) * 255, 255);
  byte bval = (intensity * b) / 255;//min( (intensity * b) * 255, 255);

  pixels.setPixelColor(led, pixels.Color(rval, gval, bval));
  pixels.show();   // Send the updated pixel colors to the hardware.

  
/*
  Serial.print("writeRgbLED called, led: ");Serial.print(led);
  Serial.print(" r: ");Serial.print(rval);
  Serial.print(", g: ");Serial.print(gval);
  Serial.print(", b: ");Serial.print(bval);DPRINTLN("");
  */
}
#endif //NEOPIXELS_ENABLED

#define MAIN_ENGINE_BASE_COLOR1_R  62
#define MAIN_ENGINE_BASE_COLOR1_G  98
#define MAIN_ENGINE_BASE_COLOR1_B  149

#define MAIN_ENGINE_BASE_COLOR2_R  240
#define MAIN_ENGINE_BASE_COLOR2_G  255  
#define MAIN_ENGINE_BASE_COLOR2_B  255

#define MINI_ENGINE_BASE_COLOR1_R  212
#define MINI_ENGINE_BASE_COLOR1_G  204
#define MINI_ENGINE_BASE_COLOR1_B  198

#define MINI_ENGINE_BASE_COLOR2_R  254
#define MINI_ENGINE_BASE_COLOR2_G  255
#define MINI_ENGINE_BASE_COLOR2_B  255


#define MAIN_LIGHT_COLOR1_R  245
#define MAIN_LIGHT_COLOR1_G  248
#define MAIN_LIGHT_COLOR1_B  236

#define MAIN_LIGHT_COLOR2_R  255
#define MAIN_LIGHT_COLOR2_G  255
#define MAIN_LIGHT_COLOR2_B  255

#define MAIN_ENGINE_RANGE   100
#define MINI_ENGINE_RANGE   200

struct rgb_color {
  enum {
    MAX_COLOR_VAL = 255  
  };
  byte r;
  byte g;
  byte b;
  
  rgb_color():r(0),g(0),b(0) {}

  rgb_color(byte rr, byte gg, byte bb) :r(rr), g(gg), b(bb) {}
};

struct MainEngineBaseColor1 : public rgb_color {
    MainEngineBaseColor1() :rgb_color (){
        r = MAIN_ENGINE_BASE_COLOR1_R;
        g = MAIN_ENGINE_BASE_COLOR1_G;
        b = MAIN_ENGINE_BASE_COLOR1_B;
    }
};

struct MiniEngineBaseColor1 : public rgb_color {
    MiniEngineBaseColor1() :rgb_color() {
        r = MINI_ENGINE_BASE_COLOR1_R;
        g = MINI_ENGINE_BASE_COLOR1_G;
        b = MINI_ENGINE_BASE_COLOR1_B;
    }
};



struct MainEngineBaseColor2 : public rgb_color {
    MainEngineBaseColor2() :rgb_color() {
        r = MAIN_ENGINE_BASE_COLOR2_R;
        g = MAIN_ENGINE_BASE_COLOR2_G;
        b = MAIN_ENGINE_BASE_COLOR2_B;
    }
};

struct MiniEngineBaseColor2 : public rgb_color {
    MiniEngineBaseColor2() :rgb_color() {
        r = MINI_ENGINE_BASE_COLOR2_R;
        g = MINI_ENGINE_BASE_COLOR2_G;
        b = MINI_ENGINE_BASE_COLOR2_B;
    }
};

void lerp_rgb_color(const rgb_color& a, const rgb_color& b, rgb_color& result, float amt)
{
    result.r = a.r + ((b.r - a.r) * amt)/255;
    result.g = a.g + ((b.g - a.g) * amt) / 255;
    result.b = a.b + ((b.b - a.b) * amt) / 255;
}

class RgbLed : public LedController {
  public:
  
  byte intensity;
#if NEOPIXELS_ENABLED
  static Adafruit_NeoPixel* g_pixelsPtr;
#endif //NEOPIXELS_ENABLED
  
  RgbLed():
    intensity(255)    
    {
    
  }

  virtual ~RgbLed() {
    
  }

#if NEOPIXELS_ENABLED
  void init(Adafruit_NeoPixel& pixels, byte index, unsigned long st ) {
      g_pixelsPtr = &pixels;
      setStartTime(st);
    setIdx(index);
    intensity = 0;      
    setModified(true);
  }
#endif //NEOPIXELS_ENABLED

  virtual int getIntensity() { return intensity; }

  void setIntensity( byte i ) {
    if (intensity != i) {
      intensity = i;
      setModified(true);
    }
  }

  void incrIntensity(byte intensityIncr) {
    if (intensity < 255) {
      intensity = min(intensity + intensityIncr,255);
      setModified(true);
    }
  }

  virtual void preUpdate(unsigned long timeFromStart, byte intensityIncr) {
    incrIntensity(intensityIncr);
  }

  void stop() {
    setIntensity(0);

    

    if (g_pixelsPtr) {
      writeRgbLED(*g_pixelsPtr, getIdx(),0,0,0,0);

      //Serial.print(getIdx());
      //Serial.println(" ~");
    }
    setModified(false);
  }
  
  
};


class Engine : public RgbLed {
  public:
    rgb_color color;
  byte r, g, b;
  
  byte idxOffset;

  Engine():
    RgbLed(),r(0),g(0),b(0), idxOffset(0){
    
  } 

  virtual ~Engine() {
    
  }
  
  virtual void ledFullOn() {
      if (g_delegatePtr) {
          g_delegatePtr->onEngineLedFullOn(this);
      }
  }

  virtual void ledStarted() {
      if (g_delegatePtr) {
          g_delegatePtr->onEngineLedStarted(this);
      }
  }

  void init(Adafruit_NeoPixel& pixels, int index, int offset, byte rr, byte gg, byte bb, unsigned long st) {
      g_pixelsPtr = &pixels;
      setStartTime(st);
      setIdx(index+ offset);
      idxOffset = offset;
      
      color.r = color.g = color.b = 0;
      intensity = 0;
      setColor(rr, gg, bb);
      setModified(true);
  }

  void setColor(byte rv, byte gv, byte bv) {
      if (rv != color.r) {
          color.r = rv;
          setModified(true);
      }
      if (gv != color.g) {
          color.g = gv;
          setModified(true);
      }
      if (bv != color.b) {
          color.b = bv;
          setModified(true);
      }
  }

  void setRGB(byte rv, byte gv, byte bv) {
      r = rv;
      g = gv;
      b = bv;
      setColor(r, g, b);
      setModified(true);
  }

#define PULSE_RANGE 50

  void pulse() {
    int rv = max(0, min(r + (((rand() % PULSE_RANGE) - (PULSE_RANGE /2))), 255));
    int gv = max(0, min(g + (((rand() % PULSE_RANGE) - (PULSE_RANGE /2))), 255));
    int bv = max(0, min(b + (((rand() % PULSE_RANGE) - (PULSE_RANGE /2))), 255));
    
    setColor(rv,gv,bv);
  }

  virtual void preUpdate(unsigned long timeFromStart, byte intensityIncr) {
    incrIntensity(intensityIncr);
    byte idx = getIdx()- idxOffset;
    //Serial.print("|E"); Serial.print(getIdx()); Serial.print(","); Serial.print(intensity); Serial.println("|");
    if (intensity < 255) {
        rgb_color res;
        if (idx == LEFT_ENGINE || idx == CENTER_ENGINE|| idx == RIGHT_ENGINE) {
            MainEngineBaseColor1 c1; MainEngineBaseColor2 c2;
            lerp_rgb_color(c1, c2, res, intensity);
        }
        else {
            MiniEngineBaseColor1 c1; MiniEngineBaseColor2 c2;
            lerp_rgb_color(c1, c2, res, intensity);
        }
        setColor(res.r, res.g, res.b);
    }
    else {
        pulse();
    }
  }

  
  virtual void updateLed() {
#if NEOPIXELS_ENABLED    
      if (g_pixelsPtr) {
          writeRgbLED(*g_pixelsPtr, getIdx()/* + idxOffset */, color.r, color.g, color.b, intensity);
          //Serial.print("+El"); Serial.print(this->getIdx());
      }
#endif //NEOPIXELS_ENABLED
  }

  
};





class ISDEngines : public LedCollection {
public:

  enum {
    ENGINE_COUNT = 7,
    MAIN_ENGINE_START_OFFSET = 2000,
    MAIN_ENGINE_START = 5000,
    MINI_ENGINE_START = 3400,
  };
  
  Engine* engines[ENGINE_COUNT];
  //int engineIdxOffset;
  

  ISDEngines():
    LedCollection()
    //engineIdxOffset(0) 
  {  
    for (int i=0;i<ENGINE_COUNT;i++ ) {
      engines[i] = new Engine();
    }
  }

  ~ISDEngines() {
    for (int i=0;i<ENGINE_COUNT;i++ ) {
      delete engines[i];
    }
  }

  static size_t sizeOf() {
      return sizeof(ISDEngines) + sizeof(Engine) * ISDEngines::ENGINE_COUNT;
  }

  void stop() {
    for (int i=0;i<ENGINE_COUNT;i++ ) {
      engines[i]->stop();
    }
    setEnabled(false);
  }
  
  void start() {
    stop();
    for (int i = 0; i < ENGINE_COUNT; i++) {
        engines[i]->setEnabled(true);
    }
    setEnabled(true);
    startTime = millis();
  }


  void setMainEngines(bool start) {
      for (int i = 0; i < ENGINE_COUNT; i++) {
          switch (engines[i]->getIdx()) {
              case CENTER_ENGINE:case RIGHT_ENGINE:case LEFT_ENGINE: {
                  if (start) {
                      engines[i]->setEnabled(true);                      
                  }
                  else {
                      engines[i]->stop();
                      engines[i]->setEnabled(false);
                  }
              }
              break;
          }
      }
      setEnabled(true);
      startTime = millis();
  }

  void setMiniEngines(bool start) {
      for (int i = 0; i < ENGINE_COUNT; i++) {
          switch (engines[i]->getIdx()) {
            case LEFT_ENGINE_TOP:case LEFT_ENGINE_BOTTOM:
            case RIGHT_ENGINE_TOP: case RIGHT_ENGINE_BOTTOM: {
                if (start) {
                    engines[i]->setEnabled(true);
                }
                else {
                    engines[i]->stop();
                    engines[i]->setEnabled(false);
                }
            }
            break;
          }
      }
      setEnabled(true);
      startTime = millis();
  }
  void initEngines(Adafruit_NeoPixel& pixels, int offset, LedControllerEventDelegate* delegate)
  {
    //engineIdxOffset = offset;
    
    srand (time(NULL)); 

    init( &engines[0], ENGINE_COUNT );
    setIntensityIncr(2);

    int engineId = 0;
    unsigned long engStart = MAIN_ENGINE_START - MAIN_ENGINE_START_OFFSET;
    float r, g, b;
    r = g = b = 0.0;
    /*
    engine starts
    left:
    center: 
    right:

    */


    LedController::g_delegatePtr = delegate;

    for (int i=0;i<ENGINE_COUNT;i++ ) {
      Engine* engine = engines[i];

      switch(i) {
        case 0:{
          engineId = CENTER_ENGINE;
          engStart += MAIN_ENGINE_START_OFFSET;
          r = MAIN_ENGINE_BASE_COLOR1_R;
          g = MAIN_ENGINE_BASE_COLOR1_G;
          b = MAIN_ENGINE_BASE_COLOR1_B;
        }
        break;

        case 1:{
          engineId = LEFT_ENGINE;
          //engStart = (1500 - (MAIN_ENGINE_RANGE/2)) + rand() % MAIN_ENGINE_RANGE;
          engStart += MAIN_ENGINE_START_OFFSET;
          r = MAIN_ENGINE_BASE_COLOR1_R;
          g = MAIN_ENGINE_BASE_COLOR1_G;
          b = MAIN_ENGINE_BASE_COLOR1_B;
        }
        break;

        case 2:{
          engineId = RIGHT_ENGINE;
          engStart += MAIN_ENGINE_START_OFFSET;// (1800 - (MAIN_ENGINE_RANGE / 2)) + rand() % MAIN_ENGINE_RANGE;
          r = MAIN_ENGINE_BASE_COLOR1_R;
          g = MAIN_ENGINE_BASE_COLOR1_G;
          b = MAIN_ENGINE_BASE_COLOR1_B;
        }
        break;

        case 3:{
          engineId = LEFT_ENGINE_TOP;
          engStart += MAIN_ENGINE_START_OFFSET;
          r = MINI_ENGINE_BASE_COLOR1_R;
          g = MINI_ENGINE_BASE_COLOR1_G;
          b = MINI_ENGINE_BASE_COLOR1_B;
        }
        break;

        case 4:{
          engineId = LEFT_ENGINE_BOTTOM;
          //engStart = (1000 - (MINI_ENGINE_RANGE / 2)) + rand() % MINI_ENGINE_RANGE;
          engStart += MAIN_ENGINE_START_OFFSET;
          r = MINI_ENGINE_BASE_COLOR1_R;
          g = MINI_ENGINE_BASE_COLOR1_G;
          b = MINI_ENGINE_BASE_COLOR1_B;
        }
        break;

        case 5:{
          engineId = RIGHT_ENGINE_TOP;          
          //engStart = (1500 - (MINI_ENGINE_RANGE / 2)) + rand() % MINI_ENGINE_RANGE;
          engStart += MAIN_ENGINE_START_OFFSET;
          r = MINI_ENGINE_BASE_COLOR1_R;
          g = MINI_ENGINE_BASE_COLOR1_G;
          b = MINI_ENGINE_BASE_COLOR1_B;
        }
        break;

        case 6:{
          engineId = RIGHT_ENGINE_BOTTOM;
          //engStart = (2000 - (MINI_ENGINE_RANGE / 2)) + rand() % MINI_ENGINE_RANGE;
          engStart += MAIN_ENGINE_START_OFFSET;
          r = MINI_ENGINE_BASE_COLOR1_R;
          g = MINI_ENGINE_BASE_COLOR1_G;
          b = MINI_ENGINE_BASE_COLOR1_B;
        }
        break;
      }
#if NEOPIXELS_ENABLED
      engine->init(pixels, engineId,offset,
                    r,g,b, engStart);   
#endif //NEOPIXELS_ENABLED

      int range  = 40;
      int rn = (rand() % range) - (range / 2);
      int gn = (rand() % range) - (range/2);
      int bn = (rand() % range) - (range/2);

      engine->setRGB( max(0, min(255,r+rn)),
          max(0, min(255, g + gn)),
          max(0, min(255, b + bn)) );

      engine->setEnabled(false);
    }
  }
};



class ShipSection : public RgbLed {
  public:
  
  ShipSection():RgbLed() {}

  virtual ~ShipSection() {
    
  }

  virtual void ledFullOn() {
      if (g_delegatePtr) {
          g_delegatePtr->onSectionLedFullOn(this);          
      }
  }

  virtual void ledStarted() {
      if (g_delegatePtr) {
          g_delegatePtr->onSectionLedStarted(this);
      }
  }

  virtual void updateLed() {
#if NEOPIXELS_ENABLED    
      
      if (g_pixelsPtr) {
          writeRgbLED(*g_pixelsPtr, getIdx(), 
                        MAIN_LIGHT_COLOR1_R, 
              MAIN_LIGHT_COLOR1_G, 
              MAIN_LIGHT_COLOR1_B, intensity);
          //Serial.print(" intens: ");Serial.print(color.intensity);DPRINTLN(" updateLed() ");
      }
#endif //NEOPIXELS_ENABLED
  }
  
};

class ISDSections : public LedCollection {
public:

  enum Parts {
    TOWER = 8,
    TOP_SECTION = 13,
    BOTTOM_SECTION = 7,
    SECTION_COUNT = TOWER + TOP_SECTION + BOTTOM_SECTION
  };

  enum Times{
      SECTION_OFFSET = 1050
  };
  
  ShipSection* sections[ISDSections::SECTION_COUNT];//[SECTION_COUNT];
  
#if NEOPIXELS_ENABLED
  ISDSections():LedCollection(){
#else
  ISDSections(){
#endif //#if NEOPIXELS_ENABLED
      
    for (int i = 0; i < ISDSections::SECTION_COUNT; i++) {
        sections[i] = new ShipSection();
    }
  }

  ~ISDSections() {
    for (int i=0;i< ISDSections::SECTION_COUNT;i++ ) {
      delete sections[i];
    }
  }

  static size_t sizeOf() {
      return sizeof(ISDSections) + sizeof(ShipSection) * ISDSections::SECTION_COUNT;
  }

  void initSections(Adafruit_NeoPixel& pixels, LedControllerEventDelegate* delegate) {
    srand (time(NULL)); 
    
    init( &sections[0], ISDSections::SECTION_COUNT );
    
    LedController::g_delegatePtr = delegate;

    unsigned long st = SECTION_OFFSET;

    for (int i=0;i< ISDSections::SECTION_COUNT;i++ ) {
#if NEOPIXELS_ENABLED
      sections[i]->init(pixels, i,st);
#endif //NEOPIXELS_ENABLED      
      sections[i]->setEnabled(false);

      st += SECTION_OFFSET;
    }
  }

  void stop() {
    for (int i=0;i< ISDSections::SECTION_COUNT;i++ ) {
      sections[i]->stop();
    }
    setEnabled(false);
  }
  
  void start() {
    stop();
    for (int i = 0; i < ISDSections::SECTION_COUNT; i++) {
        sections[i]->setEnabled(true);
    }
    setEnabled(true);
    startTime = millis();
  }
};


class SpecialSection : public Led {
public:
    SpecialSection() {}
    virtual ~SpecialSection() {}

    virtual void ledFullOn() {
        if (g_delegatePtr) {
            g_delegatePtr->onSpecialSectionLedFullOn(this);
        }
    }

    virtual void ledStarted() {
        if (g_delegatePtr) {
            g_delegatePtr->onSpecialSectionLedStarted(this);
        }
    }
};

class ISDSpecialSections : public LedCollection {
public:

    enum Times {
        SECTION_OFFSET = 1050
    };

  enum Parts {
    GARBAGE_CHUTE = 1,
    SIDE_LIGHTS_R = 3,
    SIDE_LIGHTS_L = 3,
    DOCKING_BAY_LIGHT = 1,
    FWD_BAY_LIGHT = 1,
    SECTION_COUNT = GARBAGE_CHUTE + 
    SIDE_LIGHTS_R + SIDE_LIGHTS_L + 
    DOCKING_BAY_LIGHT +
    FWD_BAY_LIGHT,
    GARBAGE_CHUTE_IDX = 8,
  };
  
  SpecialSection* sections[ISDSpecialSections::SECTION_COUNT];
  
  ISDSpecialSections():LedCollection() {
    for (int i=0;i< ISDSpecialSections::SECTION_COUNT;i++ ) {
      sections[i] = new SpecialSection();
    }
  }


  static size_t sizeOf() {
      return sizeof(ISDSpecialSections) + sizeof(SpecialSection) * ISDSpecialSections::SECTION_COUNT;
  }

  ~ISDSpecialSections() {
    for (int i=0;i< ISDSpecialSections::SECTION_COUNT;i++ ) {
      delete sections[i];
    }
  }

  void stop() {
    for (int i=0;i< ISDSpecialSections::SECTION_COUNT;i++ ) {
      sections[i]->stop();
    }
    setEnabled(false);
  }
  
  void start() {
    stop();
    for (int i = 0; i < ISDSpecialSections::SECTION_COUNT; i++) {
        sections[i]->setEnabled(true);
    }
    setEnabled(true);
    startTime = millis();
  }

  void setGarbageChute(bool val) {
      
      for (int i = 0; i < ISDSpecialSections::SECTION_COUNT; i++) {
          int idx = sections[i]->getIdx();
          if (idx == ISDSpecialSections::GARBAGE_CHUTE_IDX) {
              sections[i]->setEnabled(val);              
              if (val) {
                  sections[i]->setStartTime(0);
                  sections[i]->setValue(255);
                  sections[i]->setIntensity(50);
              }
              else {
                  sections[i]->setStartTime(1000 + (idx * 1000));
                  sections[i]->stop();
              }
              startTime = millis();
              break;
          }
      }
      setEnabled(true);
  }
  
  void initSection(Tlc5940& tlcIc, int idx, byte ledIndex, byte value, unsigned long st) {
      if (idx < 0 || idx >= ISDSpecialSections::SECTION_COUNT) {
          return;
      }

      sections[idx]->init(tlcIc, ledIndex, value, st);
  }

  void initSections(Tlc5940& tlcIc, LedControllerEventDelegate* delegate) {
    srand (time(NULL)); 
    //DPRINTLN("ISDSpecialSections::initSections() starting...");

    init( &sections[0], ISDSpecialSections::SECTION_COUNT );
    
    LedController::g_delegatePtr = delegate;

    unsigned long st = SECTION_OFFSET;

    for (int i=0;i< ISDSpecialSections::SECTION_COUNT;i++ ) {
      #if TLC_5940_ENABLED
        if (i > 1 && i < ISDSpecialSections::SECTION_COUNT - 1) {
            sections[i]->init(tlcIc, i, 80, st);
        }
        else {
            sections[i]->init(tlcIc, i, 255, st);
        }
        
        st += SECTION_OFFSET;
      #endif //TLC_5940_ENABLED
      sections[i]->setEnabled(false);
    }
    
    setIntensityIncr(25);

    //DPRINTLN("ISDSpecialSections::initSections() done");
  }

};


class ErrorControllerDelegate {
public:
    static ErrorControllerDelegate* g_delegatePtr;
    enum ErrorCodes {
        DFPLAYER_STARTUP_ERR = 0,
        DFPLAYER_TIMEDOUT_ERR = 1,
        DFPLAYER_ERR = 2,
    };

    ErrorControllerDelegate() {}
    virtual ~ErrorControllerDelegate() {}

    virtual void onError(int errCode, int errVal) = 0;
};


class ErrorController : public Led {
public:
    unsigned long  lastChange;

    ErrorController() :Led(), lastChange(0) {
        lastChange = millis();
    }

    virtual ~ErrorController() {}

    virtual void preUpdate(unsigned long timeFromStart, byte intensityIncr) {
        
        if (timeFromStart - lastChange > 400) {
            if (value == 255) {
                setValue(0);
            }
            else if (value == 0) {
                setValue(255);
            }
            lastChange = timeFromStart;

            //Serial.print(getIdx());
            //Serial.print(" ");
            //Serial.println(value);
        }
    }
};

class ISDErrors : public LedCollection {
public:
    enum CONSTANTS {
        DFPLAYER = 0,
        SECTION_COUNT = 1
    };

    ISDErrors() :LedCollection() {
        for (int i = 0; i < ISDErrors::SECTION_COUNT; i++) {
            sections[i] = new ErrorController();
        }
    }


    ErrorController* sections[ISDErrors::SECTION_COUNT];

    virtual ~ISDErrors() {
        for (int i = 0; i < ISDErrors::SECTION_COUNT; i++) {
            delete sections[i];
        }
    }

    static size_t sizeOf() {
        return sizeof(ISDErrors) + sizeof(ErrorController) * ISDErrors::SECTION_COUNT;
    }

    void errorsOff() {
        for (int i = 0; i < ISDErrors::SECTION_COUNT; i++) {
            sections[i]->stop();
        }
        setEnabled(false);        
    }

    void signalError() {
        for (int i = 0; i < ISDErrors::SECTION_COUNT; i++) {
            sections[i]->setEnabled(true);
        }
        setEnabled(true);
        startTime = millis();
    }

    void initSections(Tlc5940& tlcIc, int offset) {
        
        init(&sections[0], ISDErrors::SECTION_COUNT);

        for (int i = 0; i < ISDErrors::SECTION_COUNT; i++) {
#if TLC_5940_ENABLED
            sections[i]->init(tlcIc, i+ offset, 255, 0);
#endif
            sections[i]->setIntensity(255);
            sections[i]->setEnabled(false);
        }
    }
};

class Sound : public BaseController {
  public:

  enum Constants{
    MAX_VOLUME = 20,
  };  
  

  byte intensity;
  byte value;
  
  bool playStarted;
  bool loopMode;
  byte playCount;
  static DFRobotDFPlayerMini* g_playerPtr;
  
  Sound():
      BaseController(),    
    intensity(0),
    value(0),    
    playStarted(false),
    loopMode(false),
    playCount(0)
    {
    
  }

  void incrIntensity(byte intensityIncr) {
    if (intensity < 255) {
      intensity = min(intensity + intensityIncr,255);
      setModified(true);
    }
  }
  

  void init(DFRobotDFPlayerMini& p, int index, byte v, unsigned long st, byte initialIntensity) {
    g_playerPtr = &p;
    setStartTime(st);
    setIdx(index);    
    intensity = initialIntensity;
    setValue(v);
    setModified(true);
  }

  void setValue(byte  v) {
    if (v != value) {
      value = v;
      setModified(true);
    }
  }

  void setLoop( bool v) {
    if (v != loopMode) {
      loopMode = v;
      setModified(true);
    }
  }
    
  void playStopped() { playStarted = false; }
  bool isPlaying() const { return true == playStarted; }

  void stop() {
      setStartTimeInvalid();
    if (g_playerPtr) {
      if (g_playerPtr->readCurrentFileNumber() == this->getIdx()) {
          //Serial.print("-s"); Serial.println(getIdx());
          g_playerPtr->pause();
      }
    }
  }
  
  void update() {
    //incrIntensity();
    
    if (isModified()) {
      if (g_playerPtr) {
          //int vol = min(value * intensity / 255, 255);
          //vol = map(vol, 0, 255, 0, MAX_VOLUME);
        
          //very expensive call!! Need tso reduce calls to this!
          //g_playerPtr->volume(vol);  //Set volume value. From 0 to 30
        //DPRINT("vol changed to "); DPRINTLN(vol);

        if (!playStarted) {
          if ( (playCount==0) || ((playCount > 0) && loopMode)) {
              g_playerPtr->play(getIdx());
            DPRINT("Playing item "); DPRINTLN(getIdx());     
            //Serial.print("*p"); Serial.print(getIdx()); Serial.println(millis());
            playStarted = true; 
            playCount ++;
          }
        }
      }
    }
    setModified(false);
  }
};


enum SoundConstants {
    IMPERIAL_MARCH = 1,
    VADERS_INTRO = 2,
    //ENGINES_STARTUP = 1,
    //SECTION_POWERUP = 2,
    FULL_START_UP = 1,
    STOP_CURRENT = 0xFF
};

class SoundControllerBase {
public:
    enum Constants {
        SERIAL_CTRL_BAUD = 9600,
        MAX_SOUND_COUNT = 5,
        DFPLAYER_INIT_TIMEOUT = 3000,
    };



    Sound* sounds[MAX_SOUND_COUNT];

    SoftwareSerial serialControl;
    DFRobotDFPlayerMini soundPlayer;

    bool playerUsable;
    byte playerVolume;

    unsigned long startTime;

    SoundControllerBase(int rxPin, int txPin) :
        serialControl(rxPin, txPin), playerUsable(false),
        playerVolume(0),
        startTime(0) {
        startTime = millis();
    }

    virtual ~SoundControllerBase() {
    }

    void stop() {
        soundPlayer.pause();
    }

    void play(int idx) {
        soundPlayer.play(idx);
    }

    byte getVolume() const { return playerVolume; }

    void setVolume(byte val) {
        if (val != playerVolume) {
            playerVolume = val;
            soundPlayer.volume(playerVolume);
        }
    }

    void init() {

        serialControl.begin(SERIAL_CTRL_BAUD);

        DPRINTLN("");
        DPRINTLN(F("i df"));
        unsigned long curr = millis();
        playerUsable = false;

        if (!soundPlayer.begin(serialControl, true, false)) {  //Use softwareSerial to communicate with mp3.
            //DPRINTLN(F("err:"));
            //DPRINTLN(F("1.Please recheck the connection!"));
            //DPRINTLN(F("2.Please insert the SD card!"));
            ErrorControllerDelegate::g_delegatePtr->onError(ErrorControllerDelegate::DFPLAYER_STARTUP_ERR, 0);
            while (true) {
                delay(0); // Code to compatible with ESP8266 watch dog.
                if ((millis() - curr) > DFPLAYER_INIT_TIMEOUT) {
                    DPRINTLN("df to");
                    playerUsable = false;
                    ErrorControllerDelegate::g_delegatePtr->onError(ErrorControllerDelegate::DFPLAYER_TIMEDOUT_ERR, 0);
                    break;
                }
            }
        }
        else {
            playerUsable = true;
        }

        DPRINTLN(F("df ok"));

        Serial.println("dfok");

        soundPlayer.EQ(DFPLAYER_EQ_CLASSIC);
        soundPlayer.volume(playerVolume);

        startTime = millis();
    }


    virtual void soundStatus(uint8_t type, int value) {
        switch (type) {
        case TimeOut:
            DPRINTLN("s: A");// F("Time Out!"));
            break;
        case WrongStack:
            DPRINTLN("s: B");// F("Stack Wrong!"));
            break;
        case DFPlayerCardInserted:
            DPRINTLN("s: C");// F("Card Inserted!"));
            break;
        case DFPlayerCardRemoved:
            DPRINTLN("s: D");// F("Card Removed!"));
            break;
        case DFPlayerCardOnline:
            DPRINTLN("s: E");// F("Card Online!"));
            break;
        case DFPlayerUSBInserted:
            DPRINTLN("s: F");
            break;
        case DFPlayerUSBRemoved:
            DPRINTLN("s: G");
            break;
        case DFPlayerPlayFinished:
            //Serial.print(F("Number:"));
            //Serial.print(value);
            //DPRINTLN(F(" Play Finished!"));
            
            break;
        case DFPlayerError:
            //Serial.print(F("DFPlayerError:"));
            ErrorControllerDelegate::g_delegatePtr->onError(ErrorControllerDelegate::DFPLAYER_ERR, value);
            switch (value) {
            case Busy:
                //DPRINTLN(F("Card not found"));
                break;
            case Sleeping:
                //DPRINTLN(F("Sleeping"));
                break;
            case SerialWrongStack:
                // DPRINTLN(F("Get Wrong Stack"));
                break;
            case CheckSumNotMatch:
                //DPRINTLN(F("Check Sum Not Match"));
                break;
            case FileIndexOut:
                //DPRINTLN(F("File Index Out of Bound"));
                //Serial.println("OOB");
                break;
            case FileMismatch:
                //DPRINTLN(F("Cannot Find File"));
                //Serial.println("no-file");
                break;
            case Advertise:
                //DPRINTLN(F("In Advertise"));
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }

};


class SoundController: public SoundControllerBase {
public:
  

  
  Sound* sounds[MAX_SOUND_COUNT];
  
  SoundController( int rxPin, int txPin ):SoundControllerBase(rxPin,txPin){
      
    for (int i=0;i<MAX_SOUND_COUNT;i++ ) {
      sounds[i] = new Sound();
    }
  }

  virtual ~SoundController() {
    for (int i=0;i<MAX_SOUND_COUNT;i++ ) {
      delete sounds[i];
    }
  }

  static size_t sizeOf() {
      return sizeof(SoundController) + (sizeof(Sound) * MAX_SOUND_COUNT);
  }

  Sound* getSound(int idx) {
    Sound* result = NULL;
    if (idx >= 0 && idx <MAX_SOUND_COUNT) {
      result = sounds[idx];
    }
    return result;  
  }

  void stopSounds() {
    for (int i=0;i<MAX_SOUND_COUNT;i++ ) {
      sounds[i]->stop();
    }
  }

  void playSound( int idx ) {
      playSoundAt(idx, 0);
  }

  void playSoundAt(int idx, unsigned long startTimeFromNow) {
      Sound* sound = getSound(idx);
      if (NULL != sound) {
          sound->setStartTime(startTimeFromNow);
          sound->setEnabled(true);
      }
  }

  void init() {
    SoundControllerBase::init();
    if (playerUsable) {
        for (int i = 0; i < MAX_SOUND_COUNT; i++) {
            Sound* sound = sounds[i];
            sound->init(soundPlayer, i, 1, -1, 1);
            sound->setEnabled(false);
        }
        sounds[0]->init(soundPlayer, 7, 1, 0, 0);
        sounds[1]->init(soundPlayer, 8, 1, 0, 0);
    }
  }

  void initSound(int index, int soundIndex, float v, int st, byte initialIntensity) {
      Sound* sound = getSound(index);
      if (sound) {
          sound->init(soundPlayer, soundIndex, v, st, initialIntensity);
      }
  }

  
  virtual void soundStatus(uint8_t type, int value){
    switch (type) {      
      case DFPlayerPlayFinished:
        //Serial.print(F("Number:"));
         // Serial.print("*pf"); Serial.println(value);
        //Serial.print(value);
        //DPRINTLN(F(" Play Finished!"));

        Sound* sound = getSound(value);
        if (sound != NULL) {
          sound->playStopped();
        }
        break;
      default:
          SoundControllerBase::soundStatus(type, value);
        break;
    } 
  }


  void loop() {    
    if (soundPlayer.available()) {
      //Print the detail message from DFPlayer to handle different errors and states.
      soundStatus(soundPlayer.readType(), soundPlayer.read()); 
    }

    if (!playerUsable) {
        //Serial.println("!pu");
        return;
    }    

    unsigned long now = millis();

    //only loop every X millseconds. simulate delay()
    int delayFactor = 20;
    unsigned long  delta = now-startTime;
    if (now % delayFactor == 0 ) {
    
      for (int i=0;i<MAX_SOUND_COUNT;i++ ) {
        Sound* sound = sounds[i];          
        if ( sound->isEnabled(delta) ) {
          sound->update(); 
          //Serial.print("*su"); Serial.println(i);
          if (sound->isPlaying()) {
            //break;//don't check anyone else, busy now
          }
        }
      }
    }    
  }
};







} //namespace libISD


#endif //_LIB_ISD_LIGHTS_AUDIO_H__

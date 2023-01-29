

#ifndef _LIB_ISD_MODEL_H__
#define _LIB_ISD_MODEL_H__

#include "lib_isd.h"


namespace libISD {

#define TEST_INIT 1


    class ISDController : public ISDControllerBleCallback, 
                            public LedControllerEventDelegate,
                            public ErrorControllerDelegate {
    public:
        enum Constants {
            /*
             * note that 3 is the default here for neopixels, but we
             * can't use that because it'salready needed by the
             * tlc5940 lib. If we try and use it here it will cause
             * a conflict and the neopixels won't work correctly
             * tlc5940 lib uses the following pins:
             * arduino    tlc5940       color code in circuit
             * 13         25            white
             * 11         26            yellow
             * 10         23            red
             * 9          24            blue
             * 3          18            green
             * these can't be changed AFAIK, unless rebuild the library
             * so for all practical purposes we need to NOT use these IO pins
             */
            NEOPIXELS_PIN = 5,
            SOFTWARE_SERIAL_RX = 7, 
            SOFTWARE_SERIAL_TX = 8,
            NEOPIXELS_LEDCOUNT = ISDEngines::ENGINE_COUNT + ISDSections::SECTION_COUNT,

            SERIAL_BAUD = 115200,
            BLE_DEVICE_BAUD = 9600,
            BLE_TX_PIN = 4, 
            BLE_RX_PIN = 2, 
            BLE_LED_STATE_PIN = A1, 
            BLE_LED_STATE_OUT = 6,
        };
#if NEOPIXELS_ENABLED
        Adafruit_NeoPixel rgbNeoPixels;
#endif //NEOPIXELS_ENABLED

        //sections 
        ISDSpecialSections* miscLights;
        ISDSections* mainLights;
        ISDEngines* engines;
        ISDErrors* errorController;
        BLEDevice* bleDevice;
        ServerI2C* audioServer;
        bool inStartupSequence;
        bool inMainLightsSequence;

        ISDController() :
            rgbNeoPixels(NEOPIXELS_LEDCOUNT, NEOPIXELS_PIN, NEO_RGB + NEO_KHZ800),
            miscLights(new ISDSpecialSections()) ,            
            mainLights(new ISDSections()),
            engines(new ISDEngines()),            
            errorController(new ISDErrors()),            
            bleDevice(new BLEDevice(4, 2, BLE_LED_STATE_PIN, BLE_LED_STATE_OUT)),
            audioServer(new ServerI2C()),
            inStartupSequence(false),
            inMainLightsSequence(false)
        {

        }

        virtual ~ISDController() {
            delete miscLights;
            delete mainLights;
            delete engines;
            delete errorController;
            delete bleDevice;
            delete audioServer;
        }

        void init() {
            ErrorControllerDelegate::g_delegatePtr = this;
            //DPRINTLN("ISDController::init() starting...");
#if TEST_INIT
            audioServer->init();
            //Serial.println("svr i");



            //neo pixels initialization

      // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
      // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
            clock_prescale_set(clock_div_1);
#endif
            // END of Trinket-specific code.
            rgbNeoPixels.begin();
            rgbNeoPixels.clear();
            //Serial.println("neo i");


            //tlc5940 init 

            Tlc.init();
            Tlc.clear();
            Tlc.update();
            //Serial.println("tlc i");

#endif

            bleDevice->begin(this, BLE_DEVICE_BAUD);
            //Serial.println("ble b");
            String s;
            bleDevice->help(s);
            //Serial.println("ble i");

            
#if TEST_INIT
            //DPRINTLN("i miscL");
            miscLights->initSections(Tlc, this);
            //Serial.println("misc i");

            errorController->initSections(Tlc, ISDSpecialSections::SECTION_COUNT);
            //Serial.println("er i");

            //DPRINTLN("i mainL");
            mainLights->initSections(rgbNeoPixels,this);
            //Serial.println("main i");

            int engineOffset = ISDSections::SECTION_COUNT;            
            //DPRINTLN("i eng");
            engines->initEngines(rgbNeoPixels, engineOffset, this);
            //Serial.println("eng i");

            //DPRINTLN("i snd");
#endif


#if TEST_INIT
            /*
            //DPRINTLN("ISDController::init() done");

            
*/

            audioServer->sendCommand(i2c::MSG_INIT_FROM_SERVER, 0);
            
            //Serial.println("svr s");

            inStartupSequence = true;
            startMainLights();
#endif
            setVolume(18);
            play(FULL_POWER_UP_INTRO_VADER);
        }

        void startMainLights() {
            mainLights->start();            
        }

        void stopMainLights() {
            inStartupSequence = false;
            mainLights->stop();
            stopAudio();
        }

        void startMiscLights() {
            miscLights->start();
        }

        void stopMiscLights() {
            inStartupSequence = false;
            miscLights->stop();
            stopAudio();
        }

        void startEngines() {
            engines->start();
        }

        void stopEngines() {
            inStartupSequence = false;
            engines->stop();
            stopAudio();
        }

        void loop() {
            
           miscLights->loop();           
            mainLights->loop();           
            engines->loop();             
            errorController->loop();
            bleDevice->loop();           
        }



        void setVolume(int val) {
            audioServer->sendCommand(i2c::MSG_UPDATE_AUDIO_VOLUME, val);            
        }

        void play(int val) {
            
            audioServer->sendCommand(i2c::MSG_PLAY, val);
        }

        void stopAudio() {
            audioServer->sendCommand(i2c::MSG_PLAY, STOP_CURRENT);
        }

        //ISDControllerBleCallback impl
        virtual void onStartPowerSequence(bool withAudio) {
            mainLights->stop();
            engines->stop();
            miscLights->stop();
            stopAudio();
            inStartupSequence = true;
            inMainLightsSequence = true;
            startMainLights();
            if (withAudio) {
                play(FULL_POWER_UP_INTRO_VADER);
            }
        }

        virtual void onUpdateAudioVolume(int val) {
            //Serial.print(">>6-"); Serial.println(val);
            setVolume(val);
        }

        virtual void onPlayImperialMarch() {
            //Serial.println(">>1");
            play(IMPERIAL_MARCH);
        }

        virtual void onPlayVadersIntro() {
            //Serial.println(">>1");
            play(VADERS_INTRO);
        }

        virtual void onGarbageChuteOn(bool val) {
            //Serial.println(">>2");

            inStartupSequence = false;
            inMainLightsSequence = false;

            miscLights->setGarbageChute(val);
        }

        virtual void onPowerUpLightSpeedEngines(bool val, bool withAudio) {
            
            //Serial.print(">>3-"); Serial.println(val);
            engines->setMiniEngines(val);
            if (val && withAudio) {
                play(LIGHTSPEED_ENGINES_AUDIO);
            }
            if (!val) {
                stopAudio();
            }
        }

        virtual void onPowerUpSubLightEngines(bool val, bool withAudio) {
            //Serial.print(">>4-"); Serial.println(val);
            engines->setMainEngines(val);
            if (val && withAudio) {
                play(MAIN_ENGINES_AUDIO);
            }
            if (!val) {
                stopAudio();
            }
            //engines->stop();
        }

        virtual void onPowerUpMainSystems(bool val, bool withAudio) {
            //Serial.println(">>5");
            if (val) {
                inMainLightsSequence = true;
                startMainLights();
                if (withAudio) {
                    play(MAIN_LIGHTING_AUDIO);
                }
            }
            else {
                inMainLightsSequence = false;
                stopMainLights();
                stopMiscLights();
                stopAudio();
            }
        }

        virtual void onEnginesStartSequence(bool withAudio) {
            inStartupSequence = false;
            inStartupSequence = false;
            startEngines();
            if (withAudio) {                
                play(FULL_ENGINES_AUDIO);
            }
            else {
                stopAudio();                
            }
        }

        virtual void onStopAudio() {
            stopAudio();
        }

        //LedControllerEventDelegate impl

        virtual void onLedFullOn(Led* led) {
            DPRINT("A ");
            DPRINTLN(led->getIdx());
            //Serial.print("-L"); Serial.print(led->getIdx()); Serial.print("|"); Serial.println(millis());
        }

        virtual void onEngineLedFullOn(Engine* led) {
            DPRINT("B ");
            DPRINTLN(led->getIdx());
           // Serial.print("-E"); Serial.print(led->getIdx()); Serial.print("|"); Serial.println(millis());

            if (inStartupSequence && led->getIdx() == 34) {
                inStartupSequence = false;
               //Serial.println("!!");
                //Serial.println(millis());
            }
        }

        virtual void onSectionLedFullOn(ShipSection* section) {
            DPRINT("C ");
            DPRINT(section->getIdx());
            DPRINT(" ");
            DPRINTLN(millis());
            //Serial.print("-S"); Serial.print(section->getIdx()); Serial.print("|"); Serial.println(millis());

            if (section->getIdx() == (ISDSections::SECTION_COUNT - 1)) {
                if (inStartupSequence || inMainLightsSequence) {
                    startMiscLights();
                }

                inMainLightsSequence = false;
            }
        }

        virtual void onSpecialSectionLedFullOn(SpecialSection* special) {
            DPRINT("D ");
            DPRINT(special->getIdx());
            DPRINT(" ");
            DPRINTLN(millis());

            //Serial.print("-s"); Serial.print(special->getIdx()); Serial.print("|"); Serial.println(millis());

            if (inStartupSequence && special->getIdx() == (ISDSpecialSections::SECTION_COUNT - 1)) {
                startEngines();
            }
        }

        virtual void onLedStarted(Led* led) {
            DPRINT("E ");
            DPRINT(millis());
            DPRINT(" idx: ");
            DPRINTLN(led->getIdx());
            //Serial.print("+L"); Serial.print(led->getIdx()); Serial.print("|"); Serial.println(millis());
        }

        virtual void onEngineLedStarted(Engine* led) {
            DPRINT("F ");
            //DPRINT(millis());
            //DPRINT(" idx: ");
            DPRINTLN(led->getIdx());
            //Serial.print("+E"); Serial.print(led->getIdx()); Serial.print("|"); Serial.println(millis());
        }

        virtual void onSectionLedStarted(ShipSection* led) {
            DPRINT("G ");
           // DPRINT(millis());
           // DPRINT(" idx: ");
            DPRINT(led->getIdx());
            DPRINT(" ");
            DPRINTLN(millis());
            //Serial.print("+S"); Serial.print(led->getIdx()); Serial.print("|"); Serial.println(millis());
        }

        virtual void onSpecialSectionLedStarted(SpecialSection* led) {
            DPRINT("H ");
            //DPRINT(millis());
            //DPRINT(" idx: ");
            DPRINT(led->getIdx());

            DPRINT(" ");
            DPRINTLN(millis());

            //Serial.print("+s"); Serial.print(led->getIdx()); Serial.print("|"); Serial.println(millis());
        }


        //ErrorControllerDelegate impl
        virtual void onError(int err, int errVal) {
            errorController->signalError();
        }
    };


   // ISDController isdModel;

}



#endif //_LIB_ISD_MODEL_H__


#ifndef _LIB_ISD_MODEL_H__
#define _LIB_ISD_MODEL_H__

#include "lib_isd.h"


namespace libISD {


    class ISDSecondaryAudioController : public ISDControllerI2CCallback {
    private:

        ClientI2C client;
        SoundControllerBase sndController;
    public:
        enum Constants {
            SOFTWARE_SERIAL_RX = 10, 
            SOFTWARE_SERIAL_TX = 11,            
        };

        ISDSecondaryAudioController():
            sndController(SOFTWARE_SERIAL_RX, SOFTWARE_SERIAL_TX) {

        }

        virtual ~ISDSecondaryAudioController() {
            
        }


        void init() {
            client.init(this);
            sndController.init();
        }

        void loop() {
        }
        

        //ISDControllerI2CCallback impl
        virtual void onUpdateAudioVolume(int val) {
            sndController.setVolume(val);
        }

        virtual void onPlay(int song) {
            if (song == STOP_CURRENT) {
                sndController.stop();
            }
            else {
                sndController.play(song);
            }
        }
    };

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

        SoundController* sndController;

        ISDErrors* errorController;

        BLEDevice* bleDevice;

        ServerI2C* server;

        bool inStartupSequence;
        bool inMainLightsSequence;

        ISDController() :
#if NEOPIXELS_ENABLED
            rgbNeoPixels(NEOPIXELS_LEDCOUNT, NEOPIXELS_PIN, NEO_RGB + NEO_KHZ800),
#endif //NEOPIXELS_ENABLED
            
            miscLights(new ISDSpecialSections()) ,
            
            mainLights(new ISDSections()),

            engines(new ISDEngines()),
            /*
            sndController(new SoundController(SOFTWARE_SERIAL_RX,SOFTWARE_SERIAL_TX)),
            errorController(new ISDErrors()),
            */
            bleDevice(new BLEDevice(4, 2, BLE_LED_STATE_PIN, BLE_LED_STATE_OUT)),
            server(new ServerI2C()),
            inStartupSequence(false),
            inMainLightsSequence(false)
        {

        }

        virtual ~ISDController() {
            delete miscLights;
            delete mainLights;
            delete engines;
            delete sndController;
            delete errorController;
            delete bleDevice;
            delete server;
        }

        void init() {
            ErrorControllerDelegate::g_delegatePtr = this;
            //DPRINTLN("ISDController::init() starting...");

#if NEOPIXELS_ENABLED
            //neo pixels initialization

      // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
      // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
            clock_prescale_set(clock_div_1);
#endif
            // END of Trinket-specific code.
            rgbNeoPixels.begin();
            rgbNeoPixels.clear();
#endif //NEOPIXELS_ENABLED

            //tlc5940 init 
#if TLC_5940_ENABLED
            Tlc.init();
            Tlc.clear();
            Tlc.update();
#endif //TLC_5940_ENABLED

            bleDevice->begin(this, BLE_DEVICE_BAUD);
            bleDevice->help();
            
            server->init();

            //DPRINTLN("i miscL");
            miscLights->initSections(Tlc, this);


            /*
            

            

            errorController->initSections(Tlc, ISDSpecialSections::SECTION_COUNT);
            */

            //DPRINTLN("i mainL");
            mainLights->initSections(rgbNeoPixels,this);
            
            int engineOffset = ISDSections::SECTION_COUNT;            
            //DPRINTLN("i eng");
            engines->initEngines(rgbNeoPixels, engineOffset, this);

            /*
            //DPRINTLN("i snd");
            sndController->init();
            
            //DPRINTLN("ISDController::init() done");

            
*/
            inStartupSequence = true;
            startMainLights();
        }

        void startMainLights() {
            mainLights->start();
        }

        void stopMainLights() {
            inStartupSequence = false;
            mainLights->stop();
            server->sendCommand(i2c::MSG_PLAY, STOP_CURRENT);
        }

        void startMiscLights() {
            miscLights->start();
        }

        void stopMiscLights() {
            inStartupSequence = false;
            miscLights->stop();
            server->sendCommand(i2c::MSG_PLAY, STOP_CURRENT);
        }

        void startEngines() {
            engines->start();
        }

        void stopEngines() {
            inStartupSequence = false;
            engines->stop();
            server->sendCommand(i2c::MSG_PLAY, STOP_CURRENT);
        }

        void loop() {
            
           miscLights->loop();
           
            mainLights->loop();
            
            engines->loop(); 
            /*
            sndController->loop();
            
            errorController->loop();
            */

            bleDevice->loop();
           
        }

        //ISDControllerBleCallback impl
        virtual void onUpdateAudioVolume(int val) {
            Serial.println(">>6");
            //sndController->setVolume(val);
        }

        virtual void onPlayImperialMarch() {
            Serial.println(">>1");
            //sndController->playSound(IMPERIAL_MARCH);
        }

        virtual void onGarbageChuteOn(bool val) {
            Serial.println(">>2");

            inStartupSequence = false;
            inMainLightsSequence = false;

            miscLights->setGarbageChute(val);
        }

        virtual void onPowerUpLightSpeedEngines(bool val) {
            Serial.println(">>3");
           // engines->setMainEngines(val);
        }

        virtual void onPowerUpSubLightEngines(bool val) {
            Serial.println(">>4");
            //engines->setMiniEngines(val);
        }

        virtual void onPowerUpMainSystems(bool val) {
            Serial.println(">>5");
            if (val) {
                inMainLightsSequence = true;
                startMainLights();
            }
            else {
                inMainLightsSequence = false;
                stopMainLights();
                stopMiscLights();
            }
        }


        //LedControllerEventDelegate impl

        virtual void onLedFullOn(Led* led) {
            DPRINT("A ");
            DPRINTLN(led->getIdx());
        }

        virtual void onEngineLedFullOn(Engine* led) {
            DPRINT("B ");
            DPRINTLN(led->getIdx());

            if (inStartupSequence && led->getIdx() == 6) {
                inStartupSequence = false;
               // Serial.println("!!");
                //Serial.println(millis());
            }
        }

        virtual void onSectionLedFullOn(ShipSection* section) {
            DPRINT("C ");
            DPRINT(section->getIdx());
            DPRINT(" ");
            DPRINTLN(millis());

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

            if (inStartupSequence && special->getIdx() == (ISDSpecialSections::SECTION_COUNT - 1)) {
                startEngines();
            }
        }

        virtual void onLedStarted(Led* led) {
            DPRINT("E ");
            DPRINT(millis());
            DPRINT(" idx: ");
            DPRINTLN(led->getIdx());
        }

        virtual void onEngineLedStarted(Engine* led) {
            DPRINT("F ");
            //DPRINT(millis());
            //DPRINT(" idx: ");
            DPRINTLN(led->getIdx());
        }

        virtual void onSectionLedStarted(ShipSection* led) {
            DPRINT("G ");
           // DPRINT(millis());
           // DPRINT(" idx: ");
            DPRINT(led->getIdx());
            DPRINT(" ");
            DPRINTLN(millis());
        }

        virtual void onSpecialSectionLedStarted(SpecialSection* led) {
            DPRINT("H ");
            //DPRINT(millis());
            //DPRINT(" idx: ");
            DPRINT(led->getIdx());

            DPRINT(" ");
            DPRINTLN(millis());
        }


        //ErrorControllerDelegate impl
        virtual void onError(int err, int errVal) {
            errorController->signalError();
        }
    };


   // ISDController isdModel;

}



#endif //_LIB_ISD_MODEL_H__
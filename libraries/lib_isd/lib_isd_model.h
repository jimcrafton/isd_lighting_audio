

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
            SERVER_INIT_LED_FREQUENCY = 500,
        };

        
        bool serverInitSent;
        bool serverPlaySent;
        unsigned long prevMillis;

        ISDSecondaryAudioController():
            sndController(SOFTWARE_SERIAL_RX, SOFTWARE_SERIAL_TX),
            serverInitSent(false),
            serverPlaySent(false),
            prevMillis(0){
            prevMillis = millis();
        }

        virtual ~ISDSecondaryAudioController() {
            
        }


        void init() {
            delay(500);

            client.init(this);
            sndController.init();
            sndController.stop();
            Serial.println("2au ok");

            digitalWrite(LED_BUILTIN, HIGH);
            delay(250);
            digitalWrite(LED_BUILTIN, LOW);
            delay(250);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(250);
            digitalWrite(LED_BUILTIN, LOW);
            delay(250);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(250);
            digitalWrite(LED_BUILTIN, LOW);
        }

        void loop() {
            if (sndController.soundPlayer.available()) {
                //Print the detail message from DFPlayer to handle different errors and states.
                sndController.soundStatus(sndController.soundPlayer.readType(), sndController.soundPlayer.read());
            }

            if (!sndController.playerUsable) {
                //Serial.println("!pu");
                return;
            }

            unsigned long now = millis();

            if (now - prevMillis > 500) {
                if (serverInitSent) {                    
                    if (digitalRead(LED_BUILTIN) == LOW) {
                        digitalWrite(LED_BUILTIN, HIGH);                        
                    }
                    else {
                        digitalWrite(LED_BUILTIN, LOW);
                    }
                }
                prevMillis = now;
            }


            /*
            //only loop every X millseconds. simulate delay()
            int delayFactor = 20;
            unsigned long  delta = now - startTime;
            if (now % delayFactor == 0) {

                for (int i = 0; i < MAX_SOUND_COUNT; i++) {
                    Sound* sound = sounds[i];
                    if (sound->isEnabled(delta)) {
                        sound->update();
                        //Serial.print("*su"); Serial.println(i);
                        if (sound->isPlaying()) {
                            //break;//don't check anyone else, busy now
                        }
                    }
                }
            }
            */
        }
        

        //ISDControllerI2CCallback impl
        virtual void onUpdateAudioVolume(int val) {
            sndController.setVolume(val);
        }

        virtual void onPlay(int song) {
            serverPlaySent = true;

            if (song == STOP_CURRENT) {
                sndController.stop();
            }
            else {
                sndController.play(song);
            }
        }

        virtual void onServerInit() {
            Serial.println("svr i");
            serverInitSent = true;
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
            
            sndController(new SoundController(SOFTWARE_SERIAL_RX,SOFTWARE_SERIAL_TX)),
            
            errorController(new ISDErrors()),
            
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
            server->init();
            Serial.println("svr i");


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
            Serial.println("neo i");
#endif //NEOPIXELS_ENABLED

            //tlc5940 init 
#if TLC_5940_ENABLED
            Tlc.init();
            Tlc.clear();
            Tlc.update();
            Serial.println("tlc i");
#endif //TLC_5940_ENABLED

            bleDevice->begin(this, BLE_DEVICE_BAUD);
            Serial.println("ble b");
            String s;
            bleDevice->help(s);
            Serial.println("ble i");

            

            //DPRINTLN("i miscL");
            miscLights->initSections(Tlc, this);
            Serial.println("misc i");

            errorController->initSections(Tlc, ISDSpecialSections::SECTION_COUNT);
            Serial.println("er i");

            //DPRINTLN("i mainL");
            mainLights->initSections(rgbNeoPixels,this);
            Serial.println("main i");

            int engineOffset = ISDSections::SECTION_COUNT;            
            //DPRINTLN("i eng");
            engines->initEngines(rgbNeoPixels, engineOffset, this);
            Serial.println("eng i");

            //DPRINTLN("i snd");
            sndController->init();
            Serial.println("snd i");
            /*
            //DPRINTLN("ISDController::init() done");

            
*/

            server->sendCommand(i2c::MSG_INIT_FROM_SERVER, 0);
            
            Serial.println("svr s");

            inStartupSequence = true;
            startMainLights();

            setVolume(18);
            playSoundAt(FULL_START_UP, 0);
        }

        void startMainLights() {
            mainLights->start();

            if (inStartupSequence) {
                server->sendCommand(i2c::MSG_UPDATE_AUDIO_VOLUME, 15);
                
                server->sendCommand(i2c::MSG_PLAY, VADERS_INTRO);
            }            
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
            
            sndController->loop();
            
            
            errorController->loop();
            

            bleDevice->loop();
           
        }



        void setVolume(int val) {
            sndController->setVolume(val);
        }

        void play(int val) {
            sndController->playSound(val);
        }

        void playSoundAt(int sound, unsigned long startTimeFromNow) {
            sndController->playSoundAt(sound, startTimeFromNow);
        }

        //ISDControllerBleCallback impl
        virtual void onUpdateAudioVolume(int val) {
            //Serial.println(">>6");
            setVolume(val);
        }

        virtual void onPlayImperialMarch() {
            //Serial.println(">>1");
            play(IMPERIAL_MARCH);
        }

        virtual void onGarbageChuteOn(bool val) {
            //Serial.println(">>2");

            inStartupSequence = false;
            inMainLightsSequence = false;

            miscLights->setGarbageChute(val);
        }

        virtual void onPowerUpLightSpeedEngines(bool val) {
            //Serial.println(">>3");
           // engines->setMainEngines(val);
        }

        virtual void onPowerUpSubLightEngines(bool val) {
            //Serial.println(">>4");
            //engines->setMiniEngines(val);
        }

        virtual void onPowerUpMainSystems(bool val) {
            //Serial.println(">>5");
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
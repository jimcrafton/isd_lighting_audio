#ifndef _LIB_ISD_AUDIO_CONTROLLER_H__
#define _LIB_ISD_AUDIO_CONTROLLER_H__

#include "lib_isd_i2c.h"

namespace libISD {



    class ISDAudioController : public ISDControllerI2CCallback,
                                public ErrorControllerDelegate {
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
        int serverInitSentCount;
        unsigned long heartbeatTimeout;

        ISDAudioController() :
            sndController(SOFTWARE_SERIAL_RX, SOFTWARE_SERIAL_TX),
            serverInitSent(false),
            serverPlaySent(false),
            prevMillis(0),
            serverInitSentCount(2),
            heartbeatTimeout(500){
            prevMillis = millis();
        }

        virtual ~ISDAudioController() {

        }


        void init() {
            ErrorControllerDelegate::g_delegatePtr = this;

            delay(500);

            client.init(this);
            sndController.init();
            sndController.stop();
            //Serial.println("2au ok");

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
            unsigned long now = millis();

            if (now - prevMillis > heartbeatTimeout) {
                
                if (digitalRead(LED_BUILTIN) == LOW) {
                    digitalWrite(LED_BUILTIN, HIGH);
                }
                else {
                    digitalWrite(LED_BUILTIN, LOW);
                }
                
                prevMillis = now;
            }
            /*
            if (sndController.soundPlayer.available()) {
                //Print the detail message from DFPlayer to handle different errors and states.
                sndController.soundStatus(sndController.soundPlayer.readType(), sndController.soundPlayer.read());
                heartbeatTimeout = 500;
            }
            else {
                //Serial.println("!sp-a");
                heartbeatTimeout = 150;
            }
            */

            if (!sndController.playerUsable) {
                Serial.println("!pu");
                return;
            }
        }


        //ISDControllerI2CCallback impl
        virtual void onUpdateAudioVolume(int val) {
            sndController.setVolume(val);
        }

        virtual void onPlay(int song) {
            Serial.println("P");
            serverPlaySent = true;

            if (song == STOP_CURRENT) {
                sndController.stop();
            }
            else {
                sndController.stop();

                sndController.play(song);
            }
        }

        virtual void onServerInit() {
            //Serial.println("svr i");
            serverInitSent = true;
        }

        //ErrorControllerDelegate impl
        virtual void onError(int errCode, int errVal) {
            Serial.print("e:"); Serial.print(errCode); Serial.print(":"); Serial.println(errVal);
        }

    };


}  //libISD





#endif //_LIB_ISD_AUDIO_CONTROLLER_H__



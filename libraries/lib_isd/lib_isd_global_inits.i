
namespace libISD {


Adafruit_NeoPixel* libISD::RgbLed::g_pixelsPtr = NULL;
Tlc5940* libISD::Led::g_tlcICPtr = NULL;
LedControllerEventDelegate* LedController::g_delegatePtr = NULL;
DFRobotDFPlayerMini* Sound::g_playerPtr = NULL;
ErrorControllerDelegate* ErrorControllerDelegate::g_delegatePtr = NULL;

}


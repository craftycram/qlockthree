#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <FastLED.h>
#include <Preferences.h>
#include "led_mapping_manager.h"

// QlockThree LED patterns and animations
enum class LEDPattern {
    OFF,
    SOLID_COLOR,
    RAINBOW,
    BREATHING,
    CLOCK_DISPLAY,
    SETUP_MODE,
    UPDATE_MODE
};

class LEDController {
public:
    LEDController();
    void begin(int pin, int numLeds, int brightness = 128);
    void update();
    
    // Pattern control
    void setPattern(LEDPattern pattern);
    void setSolidColor(CRGB color);
    void setBrightness(uint8_t brightness);
    void setSpeed(uint8_t speed);
    
    // QlockThree specific functions
    void showTime(int hours, int minutes);
    void showSetupMode();
    void showUpdateMode();
    void showWiFiConnecting();
    void showError();
    
    // Mapping management
    void setMapping(MappingType type);
    void setCustomMapping(const char* mappingId);
    LEDMappingManager* getMappingManager() { return &mappingManager; }
    
    // Utility functions
    void clear();
    void fill(CRGB color);
    void setPixel(int index, CRGB color);
    CRGB getPixel(int index);
    
    // Configuration management
    void loadSettings();
    void saveSettings();
    void setNumLeds(int count);
    void setDataPin(int pin);
    
    // Status getters
    LEDPattern getCurrentPattern() const { return currentPattern; }
    uint8_t getBrightness() const { return brightness; }
    uint8_t getSpeed() const { return speed; }
    int getNumLeds() const { return numLeds; }
    int getDataPin() const { return dataPin; }
    CRGB getSolidColor() const { return solidColor; }

private:
    Preferences preferences;
    LEDMappingManager mappingManager;
    CRGB* leds;
    bool* ledStates; // State array for mapping calculations
    int numLeds;
    int dataPin;
    uint8_t brightness;
    uint8_t speed;
    LEDPattern currentPattern;
    CRGB solidColor;
    
    // Animation state
    unsigned long lastUpdate;
    uint8_t animationStep;
    uint16_t hue;
    
    // Pattern implementations
    void updateRainbow();
    void updateBreathing();
    void updateClockDisplay();
    void updateSetupMode();
    void updateUpdateMode();
    
    // QlockThree word mapping (example for German)
    void illuminateWord(int startLed, int length);
    void showHour(int hour);
    void showMinute(int minute);
};

#endif // LED_CONTROLLER_H

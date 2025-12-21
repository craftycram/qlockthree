#include "led_controller.h"

LEDController::LEDController() : 
    leds(nullptr), 
    ledStates(nullptr),
    numLeds(0), 
    dataPin(0), 
    brightness(128), 
    speed(50),
    currentPattern(LEDPattern::OFF),
    solidColor(CRGB::White),
    lastUpdate(0),
    animationStep(0),
    hue(0) {
}

void LEDController::begin(int pin, int numLedsCount, int brightnessValue) {
    // Initialize preferences
    preferences.begin("led_config", false);
    
    // Load saved settings or use defaults
    dataPin = preferences.getInt("data_pin", pin);
    numLeds = preferences.getInt("num_leds", numLedsCount);
    brightness = preferences.getUChar("brightness", brightnessValue);
    speed = preferences.getUChar("speed", 50);
    
    // Load saved color (default to white)
    uint32_t savedColor = preferences.getUInt("solid_color", 0xFFFFFF);
    solidColor = CRGB(
        (savedColor >> 16) & 0xFF,  // Red
        (savedColor >> 8) & 0xFF,   // Green
        savedColor & 0xFF           // Blue
    );
    
    Serial.println("LED Controller settings loaded:");
    Serial.printf("- Data Pin: %d\n", dataPin);
    Serial.printf("- Num LEDs: %d\n", numLeds);
    Serial.printf("- Brightness: %d\n", brightness);
    Serial.printf("- Speed: %d\n", speed);
    
    // Allocate LED arrays
    leds = new CRGB[numLeds];
    ledStates = new bool[numLeds];
    
    // Initialize mapping manager
    mappingManager.begin();
    
    // Initialize FastLED
    FastLED.addLeds<WS2812, 0>(leds, numLeds).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(brightness);
    
    // Clear all LEDs initially
    clear();
    FastLED.show();
    
    Serial.println("LED Controller initialized:");
    Serial.printf("- Pin: %d\n", dataPin);
    Serial.printf("- LEDs: %d\n", numLeds);
    Serial.printf("- Brightness: %d\n", brightness);
    Serial.printf("- Mapping: %s\n", mappingManager.getCurrentMappingName());
}

void LEDController::update() {
    unsigned long now = millis();
    
    // Update animation based on speed setting (higher speed = faster animation)
    if (now - lastUpdate < (255 - speed) / 4) {
        return;
    }
    lastUpdate = now;
    
    switch (currentPattern) {
        case LEDPattern::OFF:
            // Already cleared, no update needed
            break;
            
        case LEDPattern::SOLID_COLOR:
            // Static color, no update needed unless color changed
            break;
            
        case LEDPattern::RAINBOW:
            updateRainbow();
            break;
            
        case LEDPattern::BREATHING:
            updateBreathing();
            break;
            
        case LEDPattern::CLOCK_DISPLAY:
            updateClockDisplay();
            break;
            
        case LEDPattern::SETUP_MODE:
            updateSetupMode();
            break;
            
        case LEDPattern::UPDATE_MODE:
            updateUpdateMode();
            break;
    }
    
    FastLED.show();
}

void LEDController::setPattern(LEDPattern pattern) {
    if (currentPattern != pattern) {
        currentPattern = pattern;
        animationStep = 0;
        hue = 0;
        
        // Initialize pattern-specific settings
        switch (pattern) {
            case LEDPattern::OFF:
                clear();
                break;
                
            case LEDPattern::SOLID_COLOR:
                fill(solidColor);
                break;
                
            case LEDPattern::RAINBOW:
            case LEDPattern::BREATHING:
            case LEDPattern::SETUP_MODE:
            case LEDPattern::UPDATE_MODE:
                // These will be handled in update()
                break;
                
            case LEDPattern::CLOCK_DISPLAY:
                clear();
                // Will be updated with actual time in update()
                break;
        }
        FastLED.show();
    }
}

void LEDController::setSolidColor(CRGB color) {
    solidColor = color;
    if (currentPattern == LEDPattern::SOLID_COLOR) {
        fill(color);
        FastLED.show();
    }
}

void LEDController::setBrightness(uint8_t brightnessValue) {
    brightness = brightnessValue;
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void LEDController::setSpeed(uint8_t speedValue) {
    speed = speedValue;
}

void LEDController::showTime(int hours, int minutes) {
    setPattern(LEDPattern::CLOCK_DISPLAY);
    clear();
    
    // Use mapping manager to calculate time display
    mappingManager.calculateTimeDisplay((uint8_t)hours, (uint8_t)minutes, ledStates);
    
    // Apply LED states to actual LEDs
    for (int i = 0; i < numLeds; i++) {
        if (ledStates[i]) {
            leds[i] = solidColor;
        } else {
            leds[i] = CRGB::Black;
        }
    }
    
    FastLED.show();
}

void LEDController::showSetupMode() {
    setPattern(LEDPattern::SETUP_MODE);
}

void LEDController::showUpdateMode() {
    setPattern(LEDPattern::UPDATE_MODE);
}

void LEDController::showWiFiConnecting() {
    // Pulsing blue pattern
    clear();
    for (int i = 0; i < numLeds; i += 4) {
        leds[i] = CRGB::Blue;
    }
    FastLED.show();
}

void LEDController::showError() {
    // Flashing red pattern
    fill(CRGB::Red);
    FastLED.show();
    delay(200);
    clear();
    FastLED.show();
    delay(200);
}

void LEDController::clear() {
    fill_solid(leds, numLeds, CRGB::Black);
}

void LEDController::fill(CRGB color) {
    fill_solid(leds, numLeds, color);
}

void LEDController::setPixel(int index, CRGB color) {
    if (index >= 0 && index < numLeds) {
        leds[index] = color;
    }
}

CRGB LEDController::getPixel(int index) {
    if (index >= 0 && index < numLeds) {
        return leds[index];
    }
    return CRGB::Black;
}

// Private animation implementations
void LEDController::updateRainbow() {
    for (int i = 0; i < numLeds; i++) {
        leds[i] = CHSV(hue + (i * 255 / numLeds), 255, 255);
    }
    hue += 2; // Speed of rainbow rotation
}

void LEDController::updateBreathing() {
    uint8_t brightness = beatsin8(30); // 30 BPM breathing
    for (int i = 0; i < numLeds; i++) {
        leds[i] = solidColor;
        leds[i].fadeToBlackBy(255 - brightness);
    }
}

void LEDController::updateClockDisplay() {
    // For QlockThree, this would update based on current time
    // This is a placeholder - implement your specific word clock logic here
}

void LEDController::updateSetupMode() {
    // Rotating blue/white pattern for setup mode
    clear();
    int pos = animationStep % numLeds;
    for (int i = 0; i < 3; i++) {
        int ledIndex = (pos + i * numLeds / 3) % numLeds;
        leds[ledIndex] = (i % 2 == 0) ? CRGB::Blue : CRGB::White;
    }
    animationStep++;
}

void LEDController::updateUpdateMode() {
    // Pulsing orange pattern for update mode
    uint8_t wave = beatsin8(60); // 60 BPM pulse
    fill(CHSV(32, 255, wave)); // Orange color
}

// QlockThree specific word mapping functions
void LEDController::illuminateWord(int startLed, int length) {
    for (int i = 0; i < length; i++) {
        if (startLed + i < numLeds) {
            leds[startLed + i] = solidColor;
        }
    }
}

void LEDController::showHour(int hour) {
    // Example word mapping for hours - customize for your QlockThree layout
    // This is a simplified example - you'd map to actual LED positions
    hour = hour % 12; // Convert to 12-hour format
    
    switch (hour) {
        case 0: case 12: illuminateWord(0, 4); break;   // "ZWÖLF" or "TWELVE"
        case 1: illuminateWord(4, 4); break;            // "EINS" or "ONE"
        case 2: illuminateWord(8, 4); break;            // "ZWEI" or "TWO"
        case 3: illuminateWord(12, 4); break;           // "DREI" or "THREE"
        case 4: illuminateWord(16, 4); break;           // "VIER" or "FOUR"
        case 5: illuminateWord(20, 4); break;           // "FÜNF" or "FIVE"
        case 6: illuminateWord(24, 5); break;           // "SECHS" or "SIX"
        case 7: illuminateWord(29, 6); break;           // "SIEBEN" or "SEVEN"
        case 8: illuminateWord(35, 4); break;           // "ACHT" or "EIGHT"
        case 9: illuminateWord(39, 4); break;           // "NEUN" or "NINE"
        case 10: illuminateWord(43, 4); break;          // "ZEHN" or "TEN"
        case 11: illuminateWord(47, 3); break;          // "ELF" or "ELEVEN"
    }
}

void LEDController::showMinute(int minute) {
    // Example minute mapping - customize for your QlockThree layout
    if (minute >= 5 && minute < 10) {
        illuminateWord(50, 4); // "FÜNF" past
    } else if (minute >= 10 && minute < 15) {
        illuminateWord(54, 4); // "ZEHN" past
    } else if (minute >= 15 && minute < 20) {
        illuminateWord(58, 7); // "VIERTEL" past
    } else if (minute >= 20 && minute < 25) {
        illuminateWord(65, 7); // "ZWANZIG" past
    } else if (minute >= 25 && minute < 30) {
        illuminateWord(50, 4); // "FÜNF" before half
        illuminateWord(72, 4); // "HALB"
    } else if (minute >= 30 && minute < 35) {
        illuminateWord(72, 4); // "HALB"
    }
    // Add more minute mappings as needed
}

// Configuration management functions
void LEDController::loadSettings() {
    if (!preferences.begin("led_config", true)) {
        Serial.println("Failed to open LED preferences");
        return;
    }
    
    // Load all settings
    int newDataPin = preferences.getInt("data_pin", dataPin);
    int newNumLeds = preferences.getInt("num_leds", numLeds);
    uint8_t newBrightness = preferences.getUChar("brightness", brightness);
    uint8_t newSpeed = preferences.getUChar("speed", speed);
    
    // Load saved color
    uint32_t savedColor = preferences.getUInt("solid_color", 0xFFFFFF);
    CRGB newSolidColor = CRGB(
        (savedColor >> 16) & 0xFF,  // Red
        (savedColor >> 8) & 0xFF,   // Green
        savedColor & 0xFF           // Blue
    );
    
    preferences.end();
    
    // Apply loaded settings
    setDataPin(newDataPin);
    setNumLeds(newNumLeds);
    setBrightness(newBrightness);
    setSpeed(newSpeed);
    setSolidColor(newSolidColor);
    
    Serial.println("LED settings loaded from NVS");
}

void LEDController::saveSettings() {
    if (!preferences.begin("led_config", false)) {
        Serial.println("Failed to open LED preferences for writing");
        return;
    }
    
    // Save all current settings
    preferences.putInt("data_pin", dataPin);
    preferences.putInt("num_leds", numLeds);
    preferences.putUChar("brightness", brightness);
    preferences.putUChar("speed", speed);
    
    // Save color as 32-bit value
    uint32_t colorValue = ((uint32_t)solidColor.r << 16) | 
                         ((uint32_t)solidColor.g << 8) | 
                         (uint32_t)solidColor.b;
    preferences.putUInt("solid_color", colorValue);
    
    preferences.end();
    
    Serial.println("LED settings saved to NVS");
}

void LEDController::setNumLeds(int count) {
    if (count != numLeds && count > 0 && count <= 500) { // Reasonable limits
        // Reallocate LED array
        if (leds) {
            delete[] leds;
        }
        
        numLeds = count;
        leds = new CRGB[numLeds];
        
        // Reinitialize FastLED with new count
        FastLED.clear();
        FastLED.addLeds<WS2812, 0>(leds, numLeds).setCorrection(TypicalLEDStrip);
        FastLED.setBrightness(brightness);
        
        clear();
        FastLED.show();
        
        Serial.printf("LED count changed to: %d\n", numLeds);
        saveSettings();
    }
}

void LEDController::setDataPin(int pin) {
    if (pin != dataPin && pin >= 0 && pin < 40) { // ESP32 GPIO limits
        dataPin = pin;
        
        // Note: Changing data pin requires restart to take effect
        // FastLED doesn't support dynamic pin changes
        Serial.printf("LED data pin changed to: %d (restart required)\n", dataPin);
        saveSettings();
    }
}

// Mapping management functions
void LEDController::setMapping(MappingType type) {
    mappingManager.loadMapping(type);
    mappingManager.saveCurrentMapping();
    
    // Update LED count if mapping specifies different count
    uint16_t mappingLEDCount = mappingManager.getCurrentMappingLEDCount();
    if (mappingLEDCount != numLeds) {
        setNumLeds(mappingLEDCount);
    }
    
    Serial.printf("LED mapping changed to: %s\n", mappingManager.getCurrentMappingName());
}

void LEDController::setCustomMapping(const char* mappingId) {
    mappingManager.setCustomMapping(mappingId);
    mappingManager.saveCurrentMapping();
    
    // Update LED count if mapping specifies different count
    uint16_t mappingLEDCount = mappingManager.getCurrentMappingLEDCount();
    if (mappingLEDCount != numLeds) {
        setNumLeds(mappingLEDCount);
    }
    
    Serial.printf("LED mapping changed to custom: %s\n", mappingManager.getCurrentMappingName());
}

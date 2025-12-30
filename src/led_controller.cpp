#include "led_controller.h"

LEDController::LEDController() : 
    leds(nullptr), 
    ledStates(nullptr),
    numLeds(0), 
    dataPin(0), 
    brightness(128), 
    speed(50),
    currentPattern(LEDPattern::OFF),
    solidColor(CRGB(255, 220, 180)),  // Neutral warm white instead of harsh pure white
    lastUpdate(0),
    animationStep(0),
    hue(0),
    startupAnimationStart(0),
    startupAnimationStep(0),
    startupAnimationComplete(false),
    wifiStatusState(0),
    timeOTAStatusState(0),
    updateStatusState(0),
    statusLEDUpdate(0),
    statusLEDStep(0),
    ledTaskHandle(nullptr),
    ledMutex(nullptr),
    taskRunning(false),
    statusLEDsEnabled(true) {
}

void LEDController::begin(int pin, int numLedsCount, int brightnessValue) {
    // Initialize preferences
    preferences.begin("led_config", false);
    
    // Load saved settings or use defaults
    dataPin = preferences.getInt("data_pin", pin);
    numLeds = preferences.getInt("num_leds", numLedsCount);
    brightness = preferences.getUChar("brightness", brightnessValue);
    speed = preferences.getUChar("speed", 50);
    
    // Load saved color (default to neutral warm white)
    uint32_t savedColor = preferences.getUInt("solid_color", 0xFFDCB4);  // RGB(255, 220, 180)
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
    
    // Initialize FastLED with GRB color order (correct for your WS2812 strips)
    FastLED.addLeds<WS2812, 0, GRB>(leds, numLeds).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(brightness);
    
    // Clear all LEDs initially
    clear();
    FastLED.show();
    
    // Enable FreeRTOS threading for non-blocking LED animations
    ledMutex = xSemaphoreCreateMutex();
    if (ledMutex == nullptr) {
        Serial.println("LED Controller: Failed to create mutex, falling back to main loop");
        taskRunning = false;
        ledTaskHandle = nullptr;
    } else {
        taskRunning = true;
        // Create LED task on core 0 (app core) with high priority
        BaseType_t result = xTaskCreatePinnedToCore(
            ledTaskFunction,     // Task function
            "LEDController",     // Task name
            4096,                // Stack size (4KB)
            this,                // Task parameter (this instance)
            2,                   // Priority (high)
            &ledTaskHandle,      // Task handle
            0                    // Core 0 (app core, core 1 is protocol core)
        );
        
        if (result == pdPASS) {
            Serial.println("LED Controller: Threading enabled - animations will run independently");
        } else {
            Serial.println("LED Controller: Failed to create task, falling back to main loop");
            taskRunning = false;
            vSemaphoreDelete(ledMutex);
            ledMutex = nullptr;
            ledTaskHandle = nullptr;
        }
    }
    
    Serial.println("LED Controller initialized:");
    Serial.printf("- Pin: %d\n", dataPin);
    Serial.printf("- LEDs: %d\n", numLeds);
    Serial.printf("- Brightness: %d\n", brightness);
    Serial.printf("- Mapping: %s\n", mappingManager.getCurrentMappingName());
    Serial.printf("- LED Task: %s\n", taskRunning ? "Running on separate thread (Core 0)" : "Using main loop updates");
}

void LEDController::update() {
    unsigned long now = millis();
    
    // Update animation based on speed setting (higher speed = faster animation)
    if (now - lastUpdate < (255 - speed) / 4) {
        // Still need to update status LEDs even if not updating main pattern
        // EXCEPT during startup animation when we want to include status LEDs in the animation
        if (currentPattern != LEDPattern::STARTUP_ANIMATION) {
            updateStatusLEDs();
        }
        FastLED.show();
        return;
    }
    lastUpdate = now;
    
    switch (currentPattern) {
        case LEDPattern::OFF:
            // Already cleared, no update needed
            break;
            
        case LEDPattern::SOLID_COLOR:
            // Apply solid color to all LEDs
            fill(solidColor);
            break;
            
        case LEDPattern::RAINBOW:
            updateRainbow();
            break;
            
        case LEDPattern::BREATHING:
            updateBreathing();
            break;
            
        case LEDPattern::CLOCK_DISPLAY:
            // Clock display is handled by showTime() method
            break;
            
        case LEDPattern::SETUP_MODE:
            updateSetupMode();
            break;
            
        case LEDPattern::UPDATE_MODE:
            updateUpdateMode();
            break;
            
        case LEDPattern::STARTUP_ANIMATION:
            updateStartupAnimation();
            break;
    }
    
    FastLED.show();
}

void LEDController::setPattern(LEDPattern pattern) {
    if (currentPattern != pattern) {
        Serial.printf("DEBUG: setPattern called - changing from %d to %d\n", (int)currentPattern, (int)pattern);
        
        // If switching away from startup animation, restore user brightness
        if (currentPattern == LEDPattern::STARTUP_ANIMATION && pattern != LEDPattern::STARTUP_ANIMATION) {
            FastLED.setBrightness(brightness);
            Serial.printf("DEBUG: Restoring brightness to %d after startup animation\n", brightness);
        }
        
        currentPattern = pattern;
        animationStep = 0;
        hue = 0;
        
        // Initialize pattern-specific settings
        switch (pattern) {
            case LEDPattern::OFF:
                clear();
                Serial.println("DEBUG: Pattern OFF - LEDs cleared");
                break;
                
            case LEDPattern::SOLID_COLOR:
                fill(solidColor);
                Serial.printf("DEBUG: Pattern SOLID_COLOR - filled with RGB(%d, %d, %d)\n", solidColor.r, solidColor.g, solidColor.b);
                break;
                
            case LEDPattern::RAINBOW:
            case LEDPattern::BREATHING:
            case LEDPattern::SETUP_MODE:
            case LEDPattern::UPDATE_MODE:
                Serial.printf("DEBUG: Pattern %d - will be handled in update()\n", (int)pattern);
                break;
                
            case LEDPattern::CLOCK_DISPLAY:
                clear();
                Serial.println("DEBUG: Pattern CLOCK_DISPLAY - LEDs cleared, will show time");
                break;
                
            case LEDPattern::STARTUP_ANIMATION:
                clear();
                startupAnimationStart = millis();
                startupAnimationStep = 0;
                startupAnimationComplete = false;
                // Set brightness to 10 for startup animation
                FastLED.setBrightness(10);
                Serial.println("DEBUG: Pattern STARTUP_ANIMATION - starting rainbow sweep at brightness 10");
                break;
        }
        FastLED.show();
        Serial.println("DEBUG: FastLED.show() called after pattern change");
    }
}

void LEDController::setSolidColor(CRGB color) {
    Serial.printf("DEBUG: setSolidColor called with RGB(%d, %d, %d)\n", color.r, color.g, color.b);
    solidColor = color;
    Serial.printf("DEBUG: Current pattern is: %d\n", (int)currentPattern);
    
    if (currentPattern == LEDPattern::SOLID_COLOR) {
        fill(color);
        FastLED.show();
        Serial.println("DEBUG: Applied solid color and updated FastLED");
    } else {
        Serial.printf("DEBUG: Color saved but not applied (pattern is %d, not SOLID_COLOR)\n", (int)currentPattern);
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
    showTime(hours, minutes, 0); // Default to Sunday if weekday not provided
}

void LEDController::showTime(int hours, int minutes, int weekday) {
    setPattern(LEDPattern::CLOCK_DISPLAY);
    
    // Use mapping manager to calculate time display WITH weekday
    mappingManager.calculateTimeDisplayWithWeekday((uint8_t)hours, (uint8_t)minutes, (uint8_t)weekday, ledStates);
    
    // Apply LED states to actual LEDs - status LEDs will override in update()
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
        leds[i] = CRGB::Cyan;
    }
    FastLED.show();
}

void LEDController::showStartupAnimation() {
    setPattern(LEDPattern::STARTUP_ANIMATION);
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
    // Clear all LEDs - status LEDs will be restored in update()
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

void LEDController::setPixelThreadSafe(int index, CRGB color) {
    if (taskRunning && ledMutex != nullptr) {
        // Take mutex before accessing LED data
        if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            setPixel(index, color);
            xSemaphoreGive(ledMutex);
        }
    } else {
        // No threading, direct access
        setPixel(index, color);
    }
}

void LEDController::showThreadSafe() {
    if (taskRunning && ledMutex != nullptr) {
        // Take mutex before updating display
        if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            FastLED.show();
            xSemaphoreGive(ledMutex);
        }
    } else {
        // No threading, direct access
        FastLED.show();
    }
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

void LEDController::updateStartupAnimation() {
    unsigned long elapsed = millis() - startupAnimationStart;
    
    // Animation duration: 1.2 seconds for full sequence + 1 second display time
    const unsigned long animationDuration = 1200; // Increased to ensure 100% completion
    const unsigned long displayDuration = 500;   // Additional time to display full animation
    const unsigned long totalDuration = animationDuration + displayDuration; // 2.2 seconds total
    
    if (elapsed >= totalDuration) {
        // Animation and display complete - turn off LEDs and mark as complete
        // Restore original brightness setting
        FastLED.setBrightness(brightness);
        setPattern(LEDPattern::OFF);
        Serial.printf("Startup animation complete - brightness restored to %d\n", brightness);
        return;
    }
    
    // Check if we're in the display phase (animation completed but showing result)
    if (elapsed >= animationDuration) {
        // Animation complete, just display the full rainbow - don't clear or update LEDs
        Serial.printf("STARTUP DEBUG: Display phase - elapsed=%lu, showing complete rainbow, mode %d\n", elapsed, currentPattern);
        // return;
    }
    
    // Clear all LEDs first
    clear();
    
    // Get startup sequence from mapping manager
    const uint8_t* ledSequence = mappingManager.getStartupSequence();
    const uint16_t totalSequenceLEDs = mappingManager.getStartupSequenceLength();
    
    if (!ledSequence || totalSequenceLEDs == 0) {
        Serial.println("STARTUP DEBUG: No startup sequence defined in mapping");
        return;
    }
    
    // Calculate how many LEDs should be lit based on time elapsed
    float progress = (float)elapsed / (float)animationDuration;
    int ledsToLight = (int)(progress * totalSequenceLEDs);
    
    // Debug: Log animation progress
    static unsigned long lastProgressDebug = 0;
    if (millis() - lastProgressDebug > 200) { // Debug every 200ms
        lastProgressDebug = millis();
        Serial.printf("STARTUP DEBUG: elapsed=%lu, progress=%.2f, ledsToLight=%d/%d\n", 
                     elapsed, progress, ledsToLight, totalSequenceLEDs);
    }
    
    // Light LEDs in sequence with rainbow colors
    for (int i = 0; i < ledsToLight && i < totalSequenceLEDs; i++) {
        // Calculate rainbow hue based on position in sequence
        uint8_t hue = (i * 255) / totalSequenceLEDs;
        
        // Get array index directly (already 0-based)
        int arrayIndex = ledSequence[i];
        
        // Set the LED with rainbow color
        if (arrayIndex >= 0 && arrayIndex < numLeds) {
            leds[arrayIndex] = CHSV(hue, 255, 255); // Full saturation and brightness
        } else {
            // Debug: Log bounds check failures
            Serial.printf("STARTUP DEBUG: LED %d out of bounds (numLeds=%d)\n", arrayIndex, numLeds);
        }
    }
}

// FreeRTOS task functions
void LEDController::ledTaskFunction(void* parameter) {
    LEDController* controller = static_cast<LEDController*>(parameter);
    controller->ledTaskLoop();
}

void LEDController::ledTaskLoop() {
    const TickType_t xDelay = pdMS_TO_TICKS(20); // 50 FPS update rate
    
    while (taskRunning) {
        // Take mutex before accessing LED data
        if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
            // Update LED patterns and animations
            update();
            
            // Release mutex
            xSemaphoreGive(ledMutex);
        }
        
        // Delay for smooth animation timing
        vTaskDelay(xDelay);
    }
    
    // Clean up task
    vTaskDelete(nullptr);
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
    
    // Load saved color (default to neutral warm white)
    uint32_t savedColor = preferences.getUInt("solid_color", 0xFFDCB4);  // RGB(255, 220, 180)
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
    preferences.end(); // Ensure any previous session is closed
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
        FastLED.addLeds<WS2812, 0, GRB>(leds, numLeds).setCorrection(TypicalLEDStrip);
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

// Status LED functions
void LEDController::setWiFiStatusLED(uint8_t state) {
    if (wifiStatusState != state) {
        wifiStatusState = state;
        Serial.printf("WiFi status LED changed to: %d\n", state);
    }
}

void LEDController::setTimeOTAStatusLED(uint8_t state) {
    if (timeOTAStatusState != state) {
        timeOTAStatusState = state;
        Serial.printf("Time/OTA status LED changed to: %d\n", state);
    }
}

void LEDController::setUpdateStatusLED(uint8_t state) {
    if (updateStatusState != state) {
        updateStatusState = state;
        Serial.printf("Update status LED changed to: %d\n", state);
    }
}

void LEDController::setStatusLEDsEnabled(bool enabled) {
    statusLEDsEnabled = enabled;
}

void LEDController::updateStatusLEDs() {
    if (!statusLEDsEnabled) {
        return; // Skip status LED updates when disabled
    }
    
    unsigned long now = millis();
    
    // Update status LEDs every 50ms for smooth breathing
    if (now - statusLEDUpdate > 50) {
        statusLEDUpdate = now;
        statusLEDStep++;
        
        // Get WiFi status LED index from mapping
        int wifiLEDIndex = mappingManager.getWiFiStatusLED();
        
        // Debug: Log when WiFi status should be breathing
        static uint8_t lastWifiState = 255;
        if (wifiStatusState != lastWifiState) {
            Serial.printf("DEBUG: WiFi LED - numLeds=%d, wifiLEDIndex=%d, state=%d\n", 
                         numLeds, wifiLEDIndex, wifiStatusState);
            lastWifiState = wifiStatusState;
        }
        
        if (numLeds > wifiLEDIndex) {
            if (wifiStatusState == 0) {
                // Off when connected
                leds[wifiLEDIndex] = CRGB::Black;
            } else if (wifiStatusState == 1) {
                // Breathing cyan while connecting
                uint8_t brightness = beatsin8(30); // 30 BPM breathing
                leds[wifiLEDIndex] = CRGB::Cyan;
                leds[wifiLEDIndex].fadeToBlackBy(255 - brightness);
                
                // Debug: Log actual LED being set
                static unsigned long lastDebugWifi = 0;
                if (millis() - lastDebugWifi > 1000) { // Debug every second
                    lastDebugWifi = millis();
                    Serial.printf("DEBUG: Setting WiFi LED (index %d) to cyan, brightness=%d\n", 
                                 wifiLEDIndex, brightness);
                }
            } else if (wifiStatusState == 2) {
                // Breathing red while in AP mode
                uint8_t brightness = beatsin8(30); // 30 BPM breathing
                leds[wifiLEDIndex] = CRGB::Red;
                leds[wifiLEDIndex].fadeToBlackBy(255 - brightness);
            }
        } else {
            // Debug: Log bounds check failure
            Serial.printf("DEBUG: WiFi LED bounds check failed - numLeds=%d, wifiLEDIndex=%d\n", 
                         numLeds, wifiLEDIndex);
        }
        
        // Get system status LED index from mapping
        int statusLEDIndex = mappingManager.getSystemStatusLED();
        if (numLeds > statusLEDIndex) {
            // Priority: Update status > OTA status > NTP status
            if (updateStatusState > 0) {
                if (updateStatusState == 1) {
                    // Breathing blue while checking for updates
                    uint8_t brightness = beatsin8(30); // 30 BPM breathing
                    leds[statusLEDIndex] = CRGB::Cyan;
                    leds[statusLEDIndex].fadeToBlackBy(255 - brightness);
                } else if (updateStatusState == 2) {
                    // Breathing purple while downloading update
                    uint8_t brightness = beatsin8(30); // 30 BPM breathing
                    leds[statusLEDIndex] = CRGB::Purple;
                    leds[statusLEDIndex].fadeToBlackBy(255 - brightness);
                } else if (updateStatusState == 3) {
                    // Flashing green three times on update success (400ms on, 400ms off)
                    if (statusLEDStep < 240) { // 3 flashes * 80 steps per flash (800ms each) = 240 steps
                        int flashCycle = (statusLEDStep / 80) % 3; // 3 cycles of 80 steps each
                        bool shouldBeOn = ((statusLEDStep % 80) < 40); // On for first 40 steps (400ms)
                        leds[statusLEDIndex] = shouldBeOn ? CRGB::Green : CRGB::Black;
                    } else {
                        leds[statusLEDIndex] = CRGB::Black;
                        updateStatusState = 0; // Reset after 3 flashes
                        statusLEDStep = 0;
                    }
                } else if (updateStatusState == 4) {
                    // Flashing red three times on update error (400ms on, 400ms off)
                    if (statusLEDStep < 240) { // 3 flashes * 80 steps per flash (800ms each) = 240 steps
                        int flashCycle = (statusLEDStep / 80) % 3; // 3 cycles of 80 steps each
                        bool shouldBeOn = ((statusLEDStep % 80) < 40); // On for first 40 steps (400ms)
                        leds[statusLEDIndex] = shouldBeOn ? CRGB::Red : CRGB::Black;
                    } else {
                        leds[statusLEDIndex] = CRGB::Black;
                        updateStatusState = 0; // Reset after 3 flashes
                        statusLEDStep = 0;
                    }
                }
                // Update status has priority - don't show OTA/NTP status
                return;
            }
            
            // Handle OTA/NTP status when update status is not active
            if (timeOTAStatusState == 0) {
                // Off
                leds[statusLEDIndex] = CRGB::Black;
            } else if (timeOTAStatusState == 1) {
                // Breathing cyan during OTA updates
                uint8_t brightness = beatsin8(30); // 30 BPM breathing
                leds[statusLEDIndex] = CRGB::Cyan;
                leds[statusLEDIndex].fadeToBlackBy(255 - brightness);
            } else if (timeOTAStatusState == 2) {
                // Flashing green three times on OTA success (400ms on, 400ms off)
                if (statusLEDStep < 240) { // 3 flashes * 80 steps per flash (800ms each) = 240 steps
                    int flashCycle = (statusLEDStep / 80) % 3; // 3 cycles of 80 steps each
                    bool shouldBeOn = ((statusLEDStep % 80) < 40); // On for first 40 steps (400ms)
                    leds[statusLEDIndex] = shouldBeOn ? CRGB::Green : CRGB::Black;
                } else {
                    leds[statusLEDIndex] = CRGB::Black;
                    timeOTAStatusState = 0; // Reset after 3 flashes
                    statusLEDStep = 0;
                }
            } else if (timeOTAStatusState == 3) {
                // Flashing red three times on OTA error (400ms on, 400ms off)
                if (statusLEDStep < 240) { // 3 flashes * 80 steps per flash (800ms each) = 240 steps
                    int flashCycle = (statusLEDStep / 80) % 3; // 3 cycles of 80 steps each
                    bool shouldBeOn = ((statusLEDStep % 80) < 40); // On for first 40 steps (400ms)
                    leds[statusLEDIndex] = shouldBeOn ? CRGB::Red : CRGB::Black;
                } else {
                    leds[statusLEDIndex] = CRGB::Black;
                    timeOTAStatusState = 0; // Reset after 3 flashes
                    statusLEDStep = 0;
                }
            } else if (timeOTAStatusState == 4) {
                // Breathing orange during NTP sync
                uint8_t brightness = beatsin8(30); // 30 BPM breathing
                leds[statusLEDIndex] = CRGB::Orange;
                leds[statusLEDIndex].fadeToBlackBy(255 - brightness);
            }
        }
    }
}

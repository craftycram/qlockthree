#include "ota_manager.h"
#include "led_controller.h"

OTAManager::OTAManager() : ledController(nullptr) {
}

void OTAManager::begin(const char* hostname, const char* password, LEDController* ledCtrl) {
    ledController = ledCtrl;
    
    ArduinoOTA.setHostname(hostname);
    
    if (password && strlen(password) > 0) {
        ArduinoOTA.setPassword(password);
    }
    
    setupCallbacks();
    ArduinoOTA.begin();
    Serial.println("OTA Ready");
}

void OTAManager::handle() {
    ArduinoOTA.handle();
}

void OTAManager::setupCallbacks() {
    ArduinoOTA.onStart([this]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {  // U_SPIFFS
            type = "filesystem";
        }
        Serial.println("Start OTA updating " + type);
        
        // Clear LEDs and prepare for progress display
        clearOTALEDs();
        
        // Set OTA status LED to indicate OTA in progress
        if (ledController) {
            ledController->setTimeOTAStatusLED(1); // Breathing cyan during OTA
            Serial.println("OTA: LED progress feedback enabled");
        }
    });
    
    ArduinoOTA.onEnd([this]() {
        Serial.println("\nOTA upload complete!");
        
        // Show completion feedback
        showOTAComplete();
        
        if (ledController) {
            ledController->setTimeOTAStatusLED(2); // Green flashing on success
        }
    });
    
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
        unsigned int percent = (progress / (total / 100));
        Serial.printf("OTA Progress: %u%%\r", percent);
        
        // Show visual progress on LEDs
        showOTAProgress(progress, total);
    });
    
    ArduinoOTA.onError([this](ota_error_t error) {
        Serial.printf("OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
        
        // Show error feedback
        if (ledController) {
            ledController->setTimeOTAStatusLED(3); // Red flashing on error
        }
        
        // Clear progress LEDs on error
        clearOTALEDs();
    });
}

void OTAManager::showOTAProgress(unsigned int progress, unsigned int total) {
    if (!ledController) {
        return; // No LED controller available
    }
    
    // Get startup sequence from mapping manager for progress display
    LEDMappingManager* mappingManager = ledController->getMappingManager();
    if (!mappingManager) {
        Serial.println("OTA: No mapping manager available for progress display");
        return;
    }
    
    const uint8_t* ledSequence = mappingManager->getStartupSequence();
    const uint16_t totalSequenceLEDs = mappingManager->getStartupSequenceLength();
    
    if (!ledSequence || totalSequenceLEDs == 0) {
        Serial.println("OTA: No startup sequence defined in mapping for progress display");
        return;
    }
    
    // Calculate how many LEDs should be lit based on upload progress
    float progressRatio = (float)progress / (float)total;
    int ledsToLight = (int)(progressRatio * totalSequenceLEDs);
    
    // Disable status LED system temporarily to take control
    ledController->setStatusLEDsEnabled(false);
    
    // Clear all LEDs first
    for (int i = 0; i < ledController->getNumLeds(); i++) {
        ledController->setPixelThreadSafe(i, CRGB::Black);
    }
    
    // Light LEDs in sequence with cyan color to show progress
    for (int i = 0; i < ledsToLight && i < totalSequenceLEDs; i++) {
        int arrayIndex = ledSequence[i];
        
        if (arrayIndex >= 0 && arrayIndex < ledController->getNumLeds()) {
            ledController->setPixelThreadSafe(arrayIndex, CRGB::Cyan); // Cyan for OTA progress
        }
    }
    
    ledController->showThreadSafe();
    
    // Debug output every 10%
    static unsigned int lastPercent = 0;
    unsigned int currentPercent = (progress * 100) / total;
    if (currentPercent >= lastPercent + 10) {
        Serial.printf("OTA: Progress %u%% - lighting %d/%d LEDs\n", currentPercent, ledsToLight, totalSequenceLEDs);
        lastPercent = (currentPercent / 10) * 10; // Round down to nearest 10%
    }
}

void OTAManager::clearOTALEDs() {
    if (!ledController) {
        return;
    }
    
    Serial.println("OTA: Clearing LEDs for upload progress display");
    
    // Disable status LED system temporarily 
    ledController->setStatusLEDsEnabled(false);
    
    // Clear all LEDs
    for (int i = 0; i < ledController->getNumLeds(); i++) {
        ledController->setPixelThreadSafe(i, CRGB::Black);
    }
    
    ledController->showThreadSafe();
}

void OTAManager::showOTAComplete() {
    if (!ledController) {
        return;
    }
    
    Serial.println("OTA: Showing completion feedback");
    
    // Get startup sequence for completion display
    LEDMappingManager* mappingManager = ledController->getMappingManager();
    if (!mappingManager) {
        return;
    }
    
    const uint8_t* ledSequence = mappingManager->getStartupSequence();
    const uint16_t totalSequenceLEDs = mappingManager->getStartupSequenceLength();
    
    if (!ledSequence || totalSequenceLEDs == 0) {
        return;
    }
    
    // Flash all progress LEDs green 3 times to show completion
    for (int flash = 0; flash < 3; flash++) {
        // Light all LEDs green
        for (int i = 0; i < totalSequenceLEDs; i++) {
            int arrayIndex = ledSequence[i];
            if (arrayIndex >= 0 && arrayIndex < ledController->getNumLeds()) {
                ledController->setPixelThreadSafe(arrayIndex, CRGB::Green);
            }
        }
        ledController->showThreadSafe();
        delay(300);
        
        // Turn off all LEDs
        for (int i = 0; i < totalSequenceLEDs; i++) {
            int arrayIndex = ledSequence[i];
            if (arrayIndex >= 0 && arrayIndex < ledController->getNumLeds()) {
                ledController->setPixelThreadSafe(arrayIndex, CRGB::Black);
            }
        }
        ledController->showThreadSafe();
        delay(300);
    }
    
    // Re-enable status LED system
    ledController->setStatusLEDsEnabled(true);
    
    Serial.println("OTA: Upload complete - LEDs cleared, status system re-enabled");
}

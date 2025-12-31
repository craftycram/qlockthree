#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "wifi_manager_helper.h"
#include "ota_manager.h"
#include "auto_updater.h"
#include "web_server_manager.h"
#include "led_controller.h"
#include "time_manager.h"
#include "birthday_manager.h"

// Create module instances
WiFiManagerHelper wifiManager;
OTAManager otaManager;
AutoUpdater autoUpdater;
WebServerManager webServer(WEB_SERVER_PORT);
LEDController ledController;
TimeManager timeManager;
BirthdayManager birthdayManager;

// Debug mode state (resets on reboot)
bool debugModeEnabled = false;
int debugHour = 12;
int debugMinute = 0;

void setup() {
    // Initialize LED Controller FIRST to ensure threading starts immediately
    // LED count will be set by the mapping manager during initialization
    ledController.begin(LED_DATA_PIN, 125, LED_BRIGHTNESS); // Default count, will be updated by mapping
    ledController.setSpeed(LED_ANIMATION_SPEED);
    
    // Initialize Serial Monitor after LED controller threading is ready
    Serial.begin(115200);
    delay(1000); // Allow serial to initialize
    
    Serial.println("Starting qlockthree with modular architecture...");
    
    // Print startup memory info
    Serial.printf("Startup free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Chip model: %s\n", ESP.getChipModel());
    Serial.printf("CPU frequency: %d MHz\n", ESP.getCpuFreqMHz());
    
    // Show beautiful startup animation (rainbow sweep for 1 second)
    Serial.println("Starting rainbow startup animation...");
    ledController.showStartupAnimation();
    
    // Animation complete, wait 500ms before turning off
    delay(2000);
    
    // Turn off LEDs after startup animation
    Serial.println("Turning off LEDs, continuing setup...");
    ledController.setPattern(LEDPattern::OFF);
    
    // Initialize WiFi Manager and start connection (non-blocking)
    wifiManager.begin(AP_SSID, AP_PASSWORD, WIFI_TIMEOUT);
    
    // Set initial WiFi status BEFORE starting connection (threading is now ready)
    ledController.setWiFiStatusLED(1); // Connecting - breathing cyan
    
    // Start WiFi connection
    wifiManager.setupWiFi();
    
    // Give LED threading time to start breathing animation
    delay(100);
    
    // Only setup other services if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        ledController.setWiFiStatusLED(0); // Connected - turn off status LED
        
        // Set NTP status LED before starting time sync
        ledController.setTimeOTAStatusLED(4); // NTP syncing - orange breathing
        
        // Initialize Time Manager
        timeManager.begin();
        
        // Check if initial time sync failed and provide visual feedback
        if (!timeManager.isTimeSynced()) {
            Serial.println("Initial NTP sync failed - showing error indication");
            // Flash red 3 times to indicate initial sync failure
            ledController.setStatusLEDsEnabled(false);
            for (int i = 0; i < 3; i++) {
                ledController.setPixel(10, CRGB::Red);
                FastLED.show();
                delay(400);
                ledController.setPixel(10, CRGB::Black);
                FastLED.show();
                delay(400);
            }
            ledController.setStatusLEDsEnabled(true);
            ledController.setTimeOTAStatusLED(4); // Keep orange breathing for retry
            Serial.println("NTP sync will retry every 30 seconds in main loop");
        } else {
            ledController.setTimeOTAStatusLED(0); // Turn off if sync succeeded
        }
        
        // Initialize OTA Manager with LED controller for progress feedback
        otaManager.begin(OTA_HOSTNAME, nullptr, &ledController);
        // Optional: otaManager.begin(OTA_HOSTNAME, OTA_PASSWORD, &ledController);
        
        // Initialize Auto Updater with LED controller for feedback
        autoUpdater.begin("craftycram/qlockthree", CURRENT_VERSION, UPDATE_CHECK_INTERVAL, &ledController);
        
        // Initialize Birthday Manager
        birthdayManager.begin();
        ledController.setBirthdayManager(&birthdayManager);

        // Initialize Web Server with TimeManager and debug state
        webServer.begin(&wifiManager, &autoUpdater, &ledController, &timeManager,
                        &debugModeEnabled, &debugHour, &debugMinute);
        webServer.setBirthdayManager(&birthdayManager);
        
        // Initial update check (show update mode during check)
        if (autoUpdater.isUpdateAvailable()) {
            ledController.showUpdateMode();
            delay(2000);
        }
        autoUpdater.checkForUpdates();
        
        // Keep LEDs off until time is synced - clock display will start when time is synced
        ledController.setPattern(LEDPattern::OFF);
        
        Serial.println("Setup complete!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Hostname: ");
        Serial.println(OTA_HOSTNAME);
        Serial.print("Current Version: ");
        Serial.println(CURRENT_VERSION);
    } else {
        Serial.println("WiFi configuration mode active - connect to " + String(AP_SSID));
    }
}

void loop() {
    // Declare static variables at the top of function
    static unsigned long lastTimeUpdate = 0;
    static bool clockStarted = false;
    static bool ntpSyncInProgress = false;
    static bool errorFlashInProgress = false;
    static unsigned long errorFlashStart = 0;
    static int errorFlashCount = 0;
    
    // Handle WiFi Manager (captive portal) - check both config mode and WiFi mode
    bool configModeActive = wifiManager.isConfigModeActive();
    wifi_mode_t wifiMode = WiFi.getMode();
    bool inAPMode = configModeActive || (wifiMode == WIFI_AP) || (wifiMode == WIFI_AP_STA);
    
    if (inAPMode) {
        ledController.setWiFiStatusLED(2); // AP mode - breathing red
        
        // Monitor memory during WiFi config portal
        static unsigned long lastMemCheck = 0;
        if (millis() - lastMemCheck > 10000) { // Every 10 seconds
            lastMemCheck = millis();
            size_t freeHeap = ESP.getFreeHeap();
            if (freeHeap < 15000) {
                Serial.printf("WARNING: Low heap during config portal: %d bytes\n", freeHeap);
            }
        }
        
        // Process WiFiManager with error handling
        try {
            wifiManager.process();
        } catch (...) {
            Serial.println("ERROR: Exception in WiFiManager::process() - restarting portal");
            delay(1000);
            ESP.restart(); // Restart to recover from WiFiManager crash
        }
        return;
    }
    
    // Check WiFi connection
    static bool wifiConnectionStarted = false;
    
    if (WiFi.status() != WL_CONNECTED) {
        if (!wifiConnectionStarted) {
            Serial.println("WiFi disconnected, attempting reconnection...");
            ledController.setWiFiStatusLED(1); // Connecting - breathing cyan
            wifiManager.setupWiFi(); // Start connection attempt
            wifiConnectionStarted = true;
        }
        
        // LEDs are updated automatically by thread - just return
        return;
    } else {
        // WiFi connected - turn off status LED and reset reconnection flag
        if (wifiConnectionStarted) {
            ledController.setWiFiStatusLED(0);
            wifiConnectionStarted = false; // Reset for next disconnection
        }
    }
    
    // Handle all services
    otaManager.handle();
    webServer.handleClient();
    
    // Check for updates ONLY AFTER time sync is complete
    static unsigned long lastUpdateCheck = 0;
    static bool initialUpdateCheckDone = false;
    static String lastVersionLogged = "";
    
    // Only start update checks after initial time sync is complete
    if (timeManager.isTimeSynced() && !initialUpdateCheckDone) {
        Serial.println("Time synced - starting initial update check...");
        ledController.setUpdateStatusLED(1); // Blue breathing - checking for updates
        
        // Force the update check (ignore interval for initial check)
        autoUpdater.checkForUpdates(true);
        
        // Get version information after check
        String latestVersion = autoUpdater.getLatestVersion();
        
        // Always log version information after check
        Serial.println("=================== UPDATE STATUS ===================");
        Serial.printf("Current Version: %s\n", CURRENT_VERSION);
        
        if (latestVersion.length() > 0) {
            Serial.printf("Latest Version: %s\n", latestVersion.c_str());
            
            if (autoUpdater.isUpdateAvailable()) {
                Serial.println("Update Status: UPDATE AVAILABLE!");
                ledController.setUpdateStatusLED(2); // Purple breathing - downloading update
                
                // Show update mode during update process
                ledController.showUpdateMode();
                
                // Perform the update
                bool updateSuccess = autoUpdater.performUpdate();
                
                if (updateSuccess) {
                    ledController.setUpdateStatusLED(3); // Green flashing - update success
                    Serial.println("Update completed successfully - device will restart");
                } else {
                    ledController.setUpdateStatusLED(4); // Red flashing - update error
                    Serial.println("Update failed - continuing with current version");
                }
            } else {
                Serial.println("Update Status: Already up to date");
                ledController.setUpdateStatusLED(0); // Turn off update LED
            }
        } else {
            Serial.println("Latest Version: Failed to retrieve version from GitHub");
            Serial.println("Update Status: Check failed - network or API error");
            ledController.setUpdateStatusLED(4); // Red flashing - update error
        }
        
        Serial.println("====================================================");
        
        initialUpdateCheckDone = true;
        lastUpdateCheck = millis(); // Reset timer for periodic checks
    }
    
    // Periodic update checks every 60 seconds (only after initial sync)
    if (timeManager.isTimeSynced() && initialUpdateCheckDone && 
        (millis() - lastUpdateCheck > 60000)) {
        lastUpdateCheck = millis();
        
        Serial.println("Starting periodic update check...");
        ledController.setUpdateStatusLED(1); // Blue breathing - checking for updates
        
        // Perform the update check
        autoUpdater.checkForUpdates();
        
        // Get version information after check
        String latestVersion = autoUpdater.getLatestVersion();
        
        // Always log version information after check
        Serial.println("=================== PERIODIC UPDATE CHECK ===================");
        Serial.printf("Current Version: %s\n", CURRENT_VERSION);
        
        if (latestVersion.length() > 0) {
            Serial.printf("Latest Version: %s\n", latestVersion.c_str());
            
            if (autoUpdater.isUpdateAvailable()) {
                Serial.println("Update Status: UPDATE AVAILABLE!");
                ledController.setUpdateStatusLED(2); // Purple breathing - downloading update
                
                // Show update mode during update process
                ledController.showUpdateMode();
                
                // Perform the update
                bool updateSuccess = autoUpdater.performUpdate();
                
                if (updateSuccess) {
                    ledController.setUpdateStatusLED(3); // Green flashing - update success
                    Serial.println("Update completed successfully - device will restart");
                } else {
                    ledController.setUpdateStatusLED(4); // Red flashing - update error
                    Serial.println("Update failed - continuing with current version");
                }
            } else {
                Serial.println("Update Status: Already up to date");
                ledController.setUpdateStatusLED(0); // Turn off update LED
            }
        } else {
            Serial.println("Latest Version: Failed to retrieve version from GitHub");
            Serial.println("Update Status: Check failed - network or API error");
            ledController.setUpdateStatusLED(4); // Red flashing - update error
        }
        
        Serial.println("================================================================");
    }
    
    // LED animations are now running in separate thread - no need for main loop updates
    // Only call manual updates during error flash when threading is temporarily disabled
    
    // qlockthree main functionality - show current time using TimeManager
    if (millis() - lastTimeUpdate > 1000) { // Update time display every second
        lastTimeUpdate = millis();
        
        // Manage NTP status LED based on sync state
        if (!timeManager.isTimeSynced() && !ntpSyncInProgress) {
            // Time not synced and no sync in progress - show orange breathing
            ledController.setTimeOTAStatusLED(4); // Orange breathing for NTP sync needed
        }
        
        // Only start clock display when time is actually synced AND valid
        time_t currentTimeSeconds = time(nullptr);
        bool hasValidTime = currentTimeSeconds > 1000000000L; // Valid timestamp
        
        if (timeManager.isTimeSynced() && hasValidTime && !clockStarted) {
            Serial.println("Time synced - starting clock display");
            ledController.setTimeOTAStatusLED(0); // Turn off NTP sync indicator
            ledController.setPattern(LEDPattern::CLOCK_DISPLAY);
            clockStarted = true;
        } else if (!hasValidTime && clockStarted) {
            // If we lose valid time, stop clock and restart sync
            Serial.println("Lost valid time - stopping clock display");
            ledController.setPattern(LEDPattern::OFF);
            clockStarted = false;
        }
        
        // Get accurate time from TimeManager (only if valid) or use debug time
        if (timeManager.isTimeSynced() && hasValidTime) {
            int hours, minutes, weekday;
            uint8_t month, day;

            if (debugModeEnabled) {
                // Use debug time override
                hours = debugHour;
                minutes = debugMinute;
                weekday = 0;  // Fixed weekday in debug mode
                month = 1;    // Fixed month in debug mode
                day = 1;      // Fixed day in debug mode
            } else {
                // Use real time
                struct tm currentTime = timeManager.getCurrentTime();
                hours = currentTime.tm_hour;
                minutes = currentTime.tm_min;
                weekday = currentTime.tm_wday;  // 0=Sunday, 1=Monday, ..., 6=Saturday
                month = currentTime.tm_mon + 1;  // tm_mon is 0-11, we need 1-12
                day = currentTime.tm_mday;
            }

            // Show time on qlockthree LEDs WITH weekday (and birthday if applicable)
            if (ledController.getCurrentPattern() == LEDPattern::CLOCK_DISPLAY) {
                bool isBirthday = birthdayManager.isBirthday(month, day);

                if (isBirthday) {
                    BirthdayManager::DisplayMode mode = birthdayManager.getDisplayMode();

                    switch (mode) {
                        case BirthdayManager::DisplayMode::REPLACE:
                            // Show only HAPPY BIRTHDAY instead of time
                            ledController.showBirthdayOnly();
                            break;

                        case BirthdayManager::DisplayMode::ALTERNATE:
                            // Alternate between time and birthday every 3 seconds
                            if (ledController.shouldShowBirthdayInAlternateMode()) {
                                ledController.showBirthdayOnly();
                            } else {
                                ledController.showTime(hours, minutes, weekday);
                            }
                            break;

                        case BirthdayManager::DisplayMode::OVERLAY:
                            // Show both time and HAPPY BIRTHDAY
                            ledController.showBirthdayOverlay(hours, minutes, weekday);
                            break;
                    }
                } else {
                    // No birthday today - show normal time
                    ledController.showTime(hours, minutes, weekday);
                }
            }
        }
        
        // Debug: Print current pattern
        static LEDPattern lastPattern = LEDPattern::OFF;
        if (ledController.getCurrentPattern() != lastPattern) {
            lastPattern = ledController.getCurrentPattern();
            Serial.printf("LED Pattern changed to: %d\n", (int)lastPattern);
        }
    }
    
    // Periodic time sync check with proper LED feedback
    static unsigned long lastSyncCheck = 0;
    if (!timeManager.isTimeSynced() && (millis() - lastSyncCheck > 30000)) { // Every 30 seconds if not synced
        lastSyncCheck = millis();
        ntpSyncInProgress = true;
        
        Serial.println("Attempting time synchronization...");
        
        // Show visual feedback DURING sync attempt - orange breathing (handled by thread)
        ledController.setTimeOTAStatusLED(4); // Orange breathing during sync
        
        bool syncSuccess = timeManager.syncTime();
        
        if (syncSuccess) {
            Serial.println("NTP sync successful!");
            ledController.setTimeOTAStatusLED(0); // Turn off status LED
            ntpSyncInProgress = false;
        } else {
            Serial.println("NTP sync failed - starting error flash sequence");
            // Start non-blocking error flash sequence
            errorFlashInProgress = true;
            errorFlashStart = millis();
            errorFlashCount = 0;
            ledController.setStatusLEDsEnabled(false); // Disable status LED system
            ntpSyncInProgress = false;
        }
    }
    
    // Handle non-blocking error flash sequence
    if (errorFlashInProgress) {
        unsigned long elapsed = millis() - errorFlashStart;
        
        // Flash pattern: 400ms on, 400ms off, repeat 3 times = 2400ms total
        int currentCycle = elapsed / 800; // Each cycle is 800ms (400 on + 400 off)
        bool shouldBeOn = (elapsed % 800) < 400; // On for first 400ms of each cycle
        
        // Debug output for first few cycles
        static unsigned long lastDebug = 0;
        if (millis() - lastDebug > 200) { // Debug every 200ms
            lastDebug = millis();
            Serial.printf("Flash debug: elapsed=%lu, cycle=%d, shouldBeOn=%s\n", 
                         elapsed, currentCycle, shouldBeOn ? "true" : "false");
        }
        
        if (currentCycle < 3) { // 3 flashes total
            if (shouldBeOn) {
                ledController.setPixelThreadSafe(10, CRGB::Red); // LED 10 at array index 10
            } else {
                ledController.setPixelThreadSafe(10, CRGB::Black);
            }
            ledController.showThreadSafe();
            
            // Debug: Log which LED is flashing
            static unsigned long lastFlashDebug = 0;
            if (millis() - lastFlashDebug > 800) { // Debug every flash cycle
                lastFlashDebug = millis();
                Serial.printf("ERROR FLASH: LED 10 (array index 10) flashing red, cycle %d/3\n", currentCycle + 1);
            }
        } else {
            // Flash sequence complete - re-enable status LED system
            Serial.printf("Error flash sequence complete after %lu ms\n", elapsed);
            Serial.println("NTP error flash: 3 red flashes on LED 10 completed");
            errorFlashInProgress = false;
            ledController.setPixelThreadSafe(10, CRGB::Black); // LED 10 at array index 10
            ledController.showThreadSafe();
            ledController.setStatusLEDsEnabled(true);
            ledController.setTimeOTAStatusLED(4); // Back to orange breathing for retry
        }
    }
    
    // Add your additional qlockthree features here:
    // - Button handling
    // - Brightness sensors  
    // - Temperature/humidity sensors
    // - Sound effects
    // - Special animations
    
    // Small delay to prevent watchdog issues
    delay(10);
}

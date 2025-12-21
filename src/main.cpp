#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "wifi_manager_helper.h"
#include "ota_manager.h"
#include "auto_updater.h"
#include "web_server_manager.h"
#include "led_controller.h"
#include "time_manager.h"

// Create module instances
WiFiManagerHelper wifiManager;
OTAManager otaManager;
AutoUpdater autoUpdater;
WebServerManager webServer(WEB_SERVER_PORT);
LEDController ledController;
TimeManager timeManager;

// Custom function for application-specific code
int myFunction(int x, int y) {
    // Your custom function implementation
    return x + y;
}

void setup() {
    // Initialize Serial Monitor
    Serial.begin(115200);
    Serial.println("Starting QlockThree with modular architecture...");
    
    // Initialize LED Controller first
    ledController.begin(LED_DATA_PIN, LED_NUM_LEDS, LED_BRIGHTNESS);
    ledController.setSpeed(LED_ANIMATION_SPEED);
    
    // Show beautiful startup animation (rainbow sweep for 1 second)
    Serial.println("Starting rainbow startup animation...");
    ledController.showStartupAnimation();
    
    // Keep the animation running for exactly 1 second while doing non-blocking initialization
    unsigned long startupStart = millis();
    while (millis() - startupStart < 1000) {
        ledController.update(); // Update the animation
        delay(10); // Small delay for smooth animation
    }
    
    // Animation complete, wait 500ms before turning off
    Serial.println("Startup animation complete, waiting 500ms...");
    delay(500);
    
    // Turn off LEDs after startup animation
    Serial.println("Turning off LEDs, continuing setup...");
    ledController.setPattern(LEDPattern::OFF);
    
    // Initialize WiFi Manager and start connection (non-blocking)
    wifiManager.begin(AP_SSID, AP_PASSWORD, WIFI_TIMEOUT);
    wifiManager.setupWiFi();
    
    // Set initial WiFi status
    ledController.setWiFiStatusLED(1); // Connecting - breathing cyan
    
    // Only setup other services if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        ledController.setWiFiStatusLED(0); // Connected - turn off status LED
        // Initialize Time Manager
        timeManager.begin();
        
        // Initialize OTA Manager
        otaManager.begin(OTA_HOSTNAME);
        // Optional: otaManager.begin(OTA_HOSTNAME, OTA_PASSWORD);
        
        // Initialize Auto Updater
        autoUpdater.begin("craftycram/qlockthree", CURRENT_VERSION, UPDATE_CHECK_INTERVAL);
        
        // Initialize Web Server with TimeManager
        webServer.begin(&wifiManager, &autoUpdater, &ledController, &timeManager);
        
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
    // Handle WiFi Manager (captive portal)
    if (wifiManager.isConfigModeActive()) {
        ledController.setWiFiStatusLED(2); // AP mode - breathing red
        ledController.showSetupMode();
        ledController.update();
        wifiManager.process();
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
        
        // Continue updating LEDs while WiFi connects
        ledController.update();
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
    
    // Check for updates and show update mode if updating
    if (autoUpdater.isUpdateAvailable()) {
        ledController.showUpdateMode();
        ledController.update();
    }
    autoUpdater.checkForUpdates(); // Checks interval internally
    
    // Update LED animations
    ledController.update();
    
    // QlockThree main functionality - show current time using TimeManager
    static unsigned long lastTimeUpdate = 0;
    static bool clockStarted = false;
    
    if (millis() - lastTimeUpdate > 1000) { // Update time display every second
        lastTimeUpdate = millis();
        
        // Start clock display only when time is synced
        if (timeManager.isTimeSynced() && !clockStarted) {
            Serial.println("Time synced - starting clock display");
            ledController.setTimeOTAStatusLED(0); // Turn off NTP sync indicator
            ledController.setPattern(LEDPattern::CLOCK_DISPLAY);
            clockStarted = true;
        }
        
        // Get accurate time from TimeManager
        if (timeManager.isTimeSynced()) {
            struct tm currentTime = timeManager.getCurrentTime();
            int hours = currentTime.tm_hour;
            int minutes = currentTime.tm_min;
            
            // Show time on QlockThree LEDs
            if (ledController.getCurrentPattern() == LEDPattern::CLOCK_DISPLAY) {
                ledController.showTime(hours, minutes);
            }
        } else {
            // Time not synced - show breathing orange
            ledController.setTimeOTAStatusLED(4);
        }
        
        // Debug: Print current pattern
        static LEDPattern lastPattern = LEDPattern::OFF;
        if (ledController.getCurrentPattern() != lastPattern) {
            lastPattern = ledController.getCurrentPattern();
            Serial.printf("LED Pattern changed to: %d\n", (int)lastPattern);
        }
    }
    
    // Periodic time sync check (every 5 minutes if not synced)
    static unsigned long lastSyncCheck = 0;
    if (!timeManager.isTimeSynced() && (millis() - lastSyncCheck > 300000)) { // 5 minutes
        lastSyncCheck = millis();
        Serial.println("Attempting time synchronization...");
        timeManager.syncTime();
    }
    
    // Add your additional QlockThree features here:
    // - Button handling
    // - Brightness sensors
    // - Temperature/humidity sensors
    // - Sound effects
    // - Special animations
    
    // Small delay to prevent watchdog issues
    delay(10);
}

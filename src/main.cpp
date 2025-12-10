#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "wifi_manager_helper.h"
#include "ota_manager.h"
#include "auto_updater.h"
#include "web_server_manager.h"
#include "led_controller.h"

// Create module instances
WiFiManagerHelper wifiManager;
OTAManager otaManager;
AutoUpdater autoUpdater;
WebServerManager webServer(WEB_SERVER_PORT);
LEDController ledController;

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
    ledController.showSetupMode(); // Show setup animation during initialization
    
    // Initialize WiFi Manager
    wifiManager.begin(AP_SSID, AP_PASSWORD, WIFI_TIMEOUT);
    wifiManager.setupWiFi();
    
    // Only setup other services if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        // Show WiFi connected status
        ledController.setSolidColor(CRGB::Green);
        ledController.setPattern(LEDPattern::SOLID_COLOR);
        delay(1000);
        
        // Initialize OTA Manager
        otaManager.begin(OTA_HOSTNAME);
        // Optional: otaManager.begin(OTA_HOSTNAME, OTA_PASSWORD);
        
        // Initialize Auto Updater
        autoUpdater.begin("craftycram/qlockthree", CURRENT_VERSION, UPDATE_CHECK_INTERVAL);
        
        // Initialize Web Server
        webServer.begin(&wifiManager, &autoUpdater, &ledController);
        
        // Initial update check (show update mode during check)
        if (autoUpdater.isUpdateAvailable()) {
            ledController.showUpdateMode();
            delay(2000);
        }
        autoUpdater.checkForUpdates();
        
        // Start showing actual time
        ledController.setPattern(LEDPattern::CLOCK_DISPLAY);
        
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
        ledController.showSetupMode();
        ledController.update();
        wifiManager.process();
        return;
    }
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, attempting reconnection...");
        ledController.showWiFiConnecting();
        wifiManager.setupWiFi();
        return;
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
    
    // QlockThree main functionality - show current time
    static unsigned long lastTimeUpdate = 0;
    if (millis() - lastTimeUpdate > 1000) { // Update time display every second
        lastTimeUpdate = millis();
        
        // Get current time (you might want to add NTP time sync later)
        // For now, using millis() to simulate time
        unsigned long totalSeconds = millis() / 1000;
        int hours = (totalSeconds / 3600) % 24;
        int minutes = (totalSeconds / 60) % 60;
        
        // Show time on QlockThree LEDs
        if (ledController.getCurrentPattern() == LEDPattern::CLOCK_DISPLAY) {
            ledController.showTime(hours, minutes);
        }
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

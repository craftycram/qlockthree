#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "wifi_manager_helper.h"
#include "ota_manager.h"
#include "auto_updater.h"
#include "web_server_manager.h"

// Create module instances
WiFiManagerHelper wifiManager;
OTAManager otaManager;
AutoUpdater autoUpdater;
WebServerManager webServer(WEB_SERVER_PORT);

// Custom function for application-specific code
int myFunction(int x, int y) {
    // Your custom function implementation
    return x + y;
}

void setup() {
    // Initialize Serial Monitor
    Serial.begin(115200);
    Serial.println("Starting QlockThree with modular architecture...");
    
    // Initialize WiFi Manager
    wifiManager.begin(AP_SSID, AP_PASSWORD, WIFI_TIMEOUT);
    wifiManager.setupWiFi();
    
    // Only setup other services if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        // Initialize OTA Manager
        otaManager.begin(OTA_HOSTNAME);
        // Optional: otaManager.begin(OTA_HOSTNAME, OTA_PASSWORD);
        
        // Initialize Auto Updater
        autoUpdater.begin("craftycram/qlockthree", CURRENT_VERSION, UPDATE_CHECK_INTERVAL);
        
        // Initialize Web Server
        webServer.begin(&wifiManager, &autoUpdater);
        
        // Initial update check
        autoUpdater.checkForUpdates();
        
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
        wifiManager.process();
        return;
    }
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, attempting reconnection...");
        wifiManager.setupWiFi();
        return;
    }
    
    // Handle all services
    otaManager.handle();
    webServer.handleClient();
    autoUpdater.checkForUpdates(); // Checks interval internally
    
    // Add your main application code here
    // Example: controlling LEDs, reading sensors, etc.
    
    // Small delay to prevent watchdog issues
    delay(10);
}

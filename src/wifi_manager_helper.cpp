#include "wifi_manager_helper.h"
#include "config.h"
#include <ESPmDNS.h>
#include "esp_heap_caps.h"
#include "esp_system.h"

// Static instance pointer
WiFiManagerHelper* WiFiManagerHelper::instance = nullptr;

WiFiManagerHelper::WiFiManagerHelper() : configModeActive(false), wifiTimeout(30000), lastHeapCheck(0) {
    instance = this;
    
    // Check for crash recovery
    esp_reset_reason_t resetReason = esp_reset_reason();
    if (resetReason == ESP_RST_PANIC || resetReason == ESP_RST_INT_WDT || resetReason == ESP_RST_TASK_WDT) {
        Serial.println("WARNING: Device recovered from crash/watchdog reset");
        Serial.printf("Reset reason: %d\n", resetReason);
        crashRecoveryMode = true;
        
        // Clear any potentially corrupted WiFi settings
        preferences.begin("qlockthree", false);
        preferences.remove("wifi_portal_params");
        Serial.println("Cleared potentially corrupted WiFi portal parameters");
    } else {
        crashRecoveryMode = false;
    }
}

void WiFiManagerHelper::begin(const char* apSSID, const char* apPassword, unsigned long timeout) {
    wifiTimeout = timeout;
    preferences.begin("qlockthree", false);
    
    // Print memory info for debugging
    printMemoryInfo("WiFiManager::begin");
    
    loadWiFiConfig();
    
    // If in crash recovery mode, use safer defaults
    if (crashRecoveryMode) {
        Serial.println("Crash recovery mode - using conservative WiFiManager settings");
    }
}

void WiFiManagerHelper::loadWiFiConfig() {
    savedSSID = preferences.getString("wifi_ssid", "");
    savedPassword = preferences.getString("wifi_password", "");
    
    Serial.println("Loaded WiFi config:");
    Serial.println("SSID: " + savedSSID);
    Serial.println("Password: [" + String(savedPassword.length() > 0 ? "SAVED" : "EMPTY") + "]");
}

void WiFiManagerHelper::saveWiFiConfig(String ssid, String password) {
    preferences.putString("wifi_ssid", ssid);
    preferences.putString("wifi_password", password);
    savedSSID = ssid;
    savedPassword = password;
    Serial.println("WiFi config saved to NVS");
}

void WiFiManagerHelper::setupWiFi() {
    // Check if we have saved credentials and hardcoded values are empty
    String ssidToUse = (strlen(WIFI_SSID) == 0) ? savedSSID : String(WIFI_SSID);
    String passToUse = (strlen(WIFI_PASSWORD) == 0) ? savedPassword : String(WIFI_PASSWORD);
    
    if (ssidToUse.length() > 0) {
        // Try to connect with saved/configured credentials
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(OTA_HOSTNAME);
        WiFi.begin(ssidToUse.c_str(), passToUse.c_str());
        
        Serial.print("Connecting to WiFi: " + ssidToUse);
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < wifiTimeout) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi connected successfully!");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            configModeActive = false;
            
            // Start mDNS
            if (!MDNS.begin(OTA_HOSTNAME)) {
                Serial.println("Error setting up MDNS responder!");
            } else {
                Serial.println("mDNS responder started");
            }
            return;
        }
    }
    
    // If we get here, WiFi connection failed or no credentials available
    Serial.println("Starting WiFi configuration portal...");
    setupWiFiManager();
}

void WiFiManagerHelper::setupWiFiManager() {
    configModeActive = true;
    
    // Print memory info before starting
    printMemoryInfo("Before WiFiManager setup");
    
    // Reset WiFi settings for fresh start
    WiFi.disconnect(true);
    delay(100); // Allow cleanup
    
    // Configure WiFiManager with memory-safe settings
    configureWiFiManagerSafely();
    
    // Start AP mode and config portal
    String apPassword = (strlen(AP_PASSWORD) > 0) ? AP_PASSWORD : "";
    
    Serial.println("Starting WiFi configuration portal...");
    printMemoryInfo("Before starting config portal");
    
    // Use conservative timeout in crash recovery mode
    if (crashRecoveryMode) {
        wifiManager.setConfigPortalTimeout(300); // 5 minutes timeout in recovery mode
        Serial.println("Crash recovery mode: Using 5-minute portal timeout");
    }
    
    wifiManager.setConfigPortalBlocking(false); // Make it non-blocking
    
    if (!wifiManager.startConfigPortal(AP_SSID, apPassword.c_str())) {
        Serial.println("Config portal started, staying in AP mode");
        printMemoryInfo("After starting config portal");
    } else {
        Serial.println("WiFi connected via portal!");
        configModeActive = false;
    }
}

void WiFiManagerHelper::process() {
    if (configModeActive) {
        // Monitor heap during processing
        monitorHeapUsage();
        
        // Check for low memory condition
        size_t freeHeap = esp_get_free_heap_size();
        if (freeHeap < 10000) { // Less than 10KB free
            Serial.printf("WARNING: Low heap during WiFiManager process: %d bytes\n", freeHeap);
            
            // Force garbage collection
            delay(10);
            
            // If still low, restart the portal
            if (esp_get_free_heap_size() < 8000) {
                Serial.println("CRITICAL: Heap too low, restarting WiFi portal");
                restartWiFiPortal();
                return;
            }
        }
        
        wifiManager.process();
    }
}

void WiFiManagerHelper::resetWiFi() {
    Serial.println("WiFi reset requested");
    
    // Clear saved WiFi credentials
    preferences.remove("wifi_ssid");
    preferences.remove("wifi_password");
    savedSSID = "";
    savedPassword = "";
    
    Serial.println("WiFi settings cleared. Device will restart and enter configuration mode.");
    delay(2000);
    ESP.restart();
}

// Static callback wrappers
void WiFiManagerHelper::saveWiFiCallbackWrapper() {
    if (instance) {
        Serial.println("WiFi configuration saved via portal");
        
        // Save the credentials
        String newSSID = WiFi.SSID();
        String newPassword = WiFi.psk();
        
        if (newSSID.length() > 0) {
            instance->saveWiFiConfig(newSSID, newPassword);
            
            // Reset config mode flag when WiFi is successfully configured
            instance->configModeActive = false;
            Serial.println("Config mode deactivated - WiFi connected successfully");
            
            // Reboot after saving WiFi credentials for clean startup
            Serial.println("Rebooting in 2 seconds to apply new WiFi settings...");
            delay(2000);
            ESP.restart();
        }
    }
}

void WiFiManagerHelper::configModeCallbackWrapper(WiFiManager* myWiFiManager) {
    Serial.println("Entered WiFi config mode");
    Serial.print("AP SSID: ");
    Serial.println(myWiFiManager->getConfigPortalSSID());
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("Connect to this AP and go to http://192.168.4.1 to configure WiFi");
}

// Memory protection and monitoring methods
void WiFiManagerHelper::printMemoryInfo(const char* context) {
    size_t freeHeap = esp_get_free_heap_size();
    size_t minFreeHeap = esp_get_minimum_free_heap_size();
    size_t largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    
    Serial.printf("=== MEMORY INFO (%s) ===\n", context);
    Serial.printf("Free heap: %d bytes\n", freeHeap);
    Serial.printf("Min free heap: %d bytes\n", minFreeHeap);
    Serial.printf("Largest free block: %d bytes\n", largestFreeBlock);
    Serial.printf("Heap fragmentation: %.1f%%\n", 100.0 - (100.0 * largestFreeBlock / freeHeap));
    Serial.println("================================");
}

void WiFiManagerHelper::monitorHeapUsage() {
    unsigned long currentTime = millis();
    if (currentTime - lastHeapCheck > 5000) { // Check every 5 seconds
        lastHeapCheck = currentTime;
        
        size_t freeHeap = esp_get_free_heap_size();
        if (freeHeap < 15000) { // Less than 15KB free
            Serial.printf("WARNING: Low heap detected: %d bytes free\n", freeHeap);
            
            // If critically low, try to free some memory
            if (freeHeap < 10000) {
                Serial.println("CRITICAL: Attempting memory cleanup");
                
                // Force string cleanup and garbage collection
                String().clear();
                delay(10);
            }
        }
    }
}

void WiFiManagerHelper::configureWiFiManagerSafely() {
    try {
        // Set up WiFiManager with conservative settings
        wifiManager.setSaveConfigCallback(saveWiFiCallbackWrapper);
        wifiManager.setAPCallback(configModeCallbackWrapper);
        
        // Configure timeouts and limits
        wifiManager.setConfigPortalTimeout(0); // Never timeout normally
        wifiManager.setConnectTimeout(30); // 30 second timeout for connecting
        wifiManager.setDebugOutput(!crashRecoveryMode); // Disable debug in recovery mode
        
        // Customize portal with memory-safe settings
        wifiManager.setTitle("qlockthree WiFi Setup");
        wifiManager.setDarkMode(true);
        
        // Only add parameters if not in crash recovery mode
        if (!crashRecoveryMode) {
            // Use static strings to avoid memory allocation issues
            static WiFiManagerParameter custom_hostname("hostname", "Device Hostname", OTA_HOSTNAME, 20, "readonly");
            static WiFiManagerParameter custom_version("version", "Firmware Version", CURRENT_VERSION, 20, "readonly");
            
            wifiManager.addParameter(&custom_hostname);
            wifiManager.addParameter(&custom_version);
            
            Serial.println("Added custom parameters to WiFiManager");
        } else {
            Serial.println("Crash recovery mode: Skipping custom parameters");
        }
        
        // Set memory-safe limits
        wifiManager.setMinimumSignalQuality(20); // Only show stronger networks
        wifiManager.setRemoveDuplicateAPs(true); // Reduce memory usage
        
        Serial.println("WiFiManager configured safely");
        printMemoryInfo("After WiFiManager configuration");
        
    } catch (...) {
        Serial.println("ERROR: Exception during WiFiManager configuration");
        crashRecoveryMode = true; // Enter recovery mode
        
        // Try with minimal configuration
        wifiManager.setSaveConfigCallback(saveWiFiCallbackWrapper);
        wifiManager.setAPCallback(configModeCallbackWrapper);
        wifiManager.setConfigPortalTimeout(300); // 5 minutes timeout
        wifiManager.setDebugOutput(false);
        Serial.println("Fallback: Minimal WiFiManager configuration applied");
    }
}

void WiFiManagerHelper::restartWiFiPortal() {
    Serial.println("Restarting WiFi portal due to memory issues");
    
    // Stop current portal
    WiFi.softAPdisconnect(true);
    delay(1000);
    
    // Clear any cached data
    wifiManager.resetSettings();
    delay(500);
    
    // Restart with minimal configuration
    crashRecoveryMode = true;
    configModeActive = false;
    
    Serial.println("WiFi portal restart complete - please reconnect");
    
    // Restart the setup process
    setupWiFiManager();
}

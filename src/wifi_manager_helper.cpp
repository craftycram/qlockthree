#include "wifi_manager_helper.h"
#include "config.h"
#include <ESPmDNS.h>

// Static instance pointer
WiFiManagerHelper* WiFiManagerHelper::instance = nullptr;

WiFiManagerHelper::WiFiManagerHelper() : configModeActive(false), wifiTimeout(30000) {
    instance = this;
}

void WiFiManagerHelper::begin(const char* apSSID, const char* apPassword, unsigned long timeout) {
    wifiTimeout = timeout;
    preferences.begin("qlockthree", false);
    loadWiFiConfig();
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
    
    // Reset WiFi settings for fresh start
    WiFi.disconnect(true);
    
    // Set up WiFiManager
    wifiManager.setSaveConfigCallback(saveWiFiCallbackWrapper);
    wifiManager.setAPCallback(configModeCallbackWrapper);
    
    // Configure WiFiManager
    wifiManager.setConfigPortalTimeout(0); // Never timeout
    wifiManager.setConnectTimeout(30); // 30 second timeout for connecting
    wifiManager.setDebugOutput(true);
    
    // Customize portal
    wifiManager.setTitle("QlockThree WiFi Setup");
    wifiManager.setDarkMode(true);
    
    // Add custom parameters
    WiFiManagerParameter custom_hostname("hostname", "Device Hostname", OTA_HOSTNAME, 20, "readonly");
    WiFiManagerParameter custom_version("version", "Firmware Version", CURRENT_VERSION, 20, "readonly");
    
    wifiManager.addParameter(&custom_hostname);
    wifiManager.addParameter(&custom_version);
    
    // Start AP mode and config portal - use timeout to make it non-blocking
    String apPassword = (strlen(AP_PASSWORD) > 0) ? AP_PASSWORD : "";
    
    Serial.println("Starting WiFi configuration portal...");
    wifiManager.setConfigPortalBlocking(false); // Make it non-blocking
    
    if (!wifiManager.startConfigPortal(AP_SSID, apPassword.c_str())) {
        Serial.println("Config portal started, staying in AP mode");
        // Config portal is now running non-blocking
    } else {
        Serial.println("WiFi connected via portal!");
        configModeActive = false;
    }
}

void WiFiManagerHelper::process() {
    if (configModeActive) {
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

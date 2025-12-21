#ifndef WIFI_MANAGER_HELPER_H
#define WIFI_MANAGER_HELPER_H

#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>

class WiFiManagerHelper {
public:
    WiFiManagerHelper();
    void begin(const char* apSSID, const char* apPassword, unsigned long timeout);
    void setupWiFi();
    bool isConfigModeActive() const { return configModeActive; }
    void process();
    void resetWiFi();
    
    // Callback setters
    void setSaveCallback(void (*callback)());
    void setConfigModeCallback(void (*callback)(WiFiManager*));

private:
    WiFiManager wifiManager;
    Preferences preferences;
    String savedSSID;
    String savedPassword;
    bool configModeActive;
    unsigned long wifiTimeout;
    
    // Memory monitoring and crash recovery
    bool crashRecoveryMode;
    unsigned long lastHeapCheck;
    
    void loadWiFiConfig();
    void saveWiFiConfig(String ssid, String password);
    void setupWiFiManager();
    
    // Memory protection methods
    void printMemoryInfo(const char* context);
    void monitorHeapUsage();
    void configureWiFiManagerSafely();
    void restartWiFiPortal();
    
    // Static callback wrappers
    static void saveWiFiCallbackWrapper();
    static void configModeCallbackWrapper(WiFiManager* myWiFiManager);
    
    // Static instance pointer for callbacks
    static WiFiManagerHelper* instance;
};

#endif // WIFI_MANAGER_HELPER_H

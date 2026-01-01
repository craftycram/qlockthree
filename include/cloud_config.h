#ifndef CLOUD_CONFIG_H
#define CLOUD_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

struct CloudSettings {
    char mqttUrl[128];          // WebSocket URL: wss://mqtt.example.com/mqtt
    char mqttUsername[64];      // Generated during provisioning
    char mqttPassword[128];     // Generated during provisioning
    char apiUrl[128];           // Backend API URL
    bool cloudEnabled;          // Whether cloud features are enabled
    bool isPaired;              // Whether device is paired to a user
    char ownerId[64];           // Keycloak user ID of owner

    CloudSettings() {
        memset(mqttUrl, 0, sizeof(mqttUrl));
        memset(mqttUsername, 0, sizeof(mqttUsername));
        memset(mqttPassword, 0, sizeof(mqttPassword));
        memset(apiUrl, 0, sizeof(apiUrl));
        cloudEnabled = false;
        isPaired = false;
        memset(ownerId, 0, sizeof(ownerId));
    }
};

class CloudConfig {
public:
    void begin();

    // Load settings from NVS
    CloudSettings load();

    // Save settings to NVS
    void save(const CloudSettings& settings);

    // Clear all cloud settings
    void clear();

    // Check if cloud is configured
    bool isConfigured();

    // Check if device is paired
    bool isPaired();

    // Save individual settings
    void setMqttCredentials(const char* url, const char* username, const char* password);
    void setApiUrl(const char* url);
    void setPaired(bool paired, const char* ownerId = nullptr);
    void setCloudEnabled(bool enabled);

private:
    Preferences preferences;
    static const char* NAMESPACE;
};

#endif // CLOUD_CONFIG_H

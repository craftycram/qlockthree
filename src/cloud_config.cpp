#include "cloud_config.h"

const char* CloudConfig::NAMESPACE = "cloud";

void CloudConfig::begin() {
    // Just initialize - actual operations open/close as needed
    Serial.println("CloudConfig initialized");
}

CloudSettings CloudConfig::load() {
    CloudSettings settings;

    preferences.begin(NAMESPACE, true); // Read-only

    preferences.getString("mqtt_url", settings.mqttUrl, sizeof(settings.mqttUrl));
    preferences.getString("mqtt_user", settings.mqttUsername, sizeof(settings.mqttUsername));
    preferences.getString("mqtt_pass", settings.mqttPassword, sizeof(settings.mqttPassword));
    preferences.getString("api_url", settings.apiUrl, sizeof(settings.apiUrl));
    preferences.getString("owner_id", settings.ownerId, sizeof(settings.ownerId));
    settings.cloudEnabled = preferences.getBool("enabled", false);
    settings.isPaired = preferences.getBool("paired", false);

    preferences.end();

    return settings;
}

void CloudConfig::save(const CloudSettings& settings) {
    preferences.begin(NAMESPACE, false); // Read-write

    preferences.putString("mqtt_url", settings.mqttUrl);
    preferences.putString("mqtt_user", settings.mqttUsername);
    preferences.putString("mqtt_pass", settings.mqttPassword);
    preferences.putString("api_url", settings.apiUrl);
    preferences.putString("owner_id", settings.ownerId);
    preferences.putBool("enabled", settings.cloudEnabled);
    preferences.putBool("paired", settings.isPaired);

    preferences.end();

    Serial.println("Cloud settings saved to NVS");
}

void CloudConfig::clear() {
    preferences.begin(NAMESPACE, false);
    preferences.clear();
    preferences.end();

    Serial.println("Cloud settings cleared");
}

bool CloudConfig::isConfigured() {
    preferences.begin(NAMESPACE, true);
    bool hasUrl = preferences.isKey("mqtt_url") && strlen(preferences.getString("mqtt_url", "").c_str()) > 0;
    bool hasCredentials = preferences.isKey("mqtt_user") && strlen(preferences.getString("mqtt_user", "").c_str()) > 0;
    preferences.end();

    return hasUrl && hasCredentials;
}

bool CloudConfig::isPaired() {
    preferences.begin(NAMESPACE, true);
    bool paired = preferences.getBool("paired", false);
    preferences.end();

    return paired;
}

void CloudConfig::setMqttCredentials(const char* url, const char* username, const char* password) {
    preferences.begin(NAMESPACE, false);
    preferences.putString("mqtt_url", url);
    preferences.putString("mqtt_user", username);
    preferences.putString("mqtt_pass", password);
    preferences.end();
}

void CloudConfig::setApiUrl(const char* url) {
    preferences.begin(NAMESPACE, false);
    preferences.putString("api_url", url);
    preferences.end();
}

void CloudConfig::setPaired(bool paired, const char* ownerId) {
    preferences.begin(NAMESPACE, false);
    preferences.putBool("paired", paired);
    if (ownerId != nullptr) {
        preferences.putString("owner_id", ownerId);
    }
    preferences.end();
}

void CloudConfig::setCloudEnabled(bool enabled) {
    preferences.begin(NAMESPACE, false);
    preferences.putBool("enabled", enabled);
    preferences.end();
}

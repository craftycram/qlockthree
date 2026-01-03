#ifndef CLOUD_MANAGER_H
#define CLOUD_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>  // Must be before MQTTPubSubClient
#include <MQTTPubSubClient.h>
#include "cloud_config.h"
#include "device_identity.h"

// Forward declarations
class LEDController;

// Cloud connection states
enum class CloudState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    PAIRING,
    ERROR
};

// Command types from cloud
enum class CloudCommandType {
    POWER,
    BRIGHTNESS,
    COLOR,
    PATTERN,
    UNPAIR,
    UNKNOWN
};

// Command callback signature
typedef void (*CloudCommandCallback)(CloudCommandType type, JsonObject& payload);

class CloudManager {
public:
    CloudManager();

    // Initialize with dependencies
    void begin(LEDController* ledController = nullptr);

    // Main loop - must be called regularly
    void loop();

    // Connection management
    bool connect();
    void disconnect();
    bool isConnected();
    CloudState getState();

    // Pairing flow
    bool startPairing(const char* apiUrl);
    void stopPairing();
    bool isPairing();
    String getPairingCode();
    int getPairingTimeRemaining(); // seconds

    // Status publishing
    void publishStatus();

    // Command handling
    void setCommandCallback(CloudCommandCallback callback);

    // Get status for web interface
    String getStatusJSON();

private:
    CloudConfig config;
    LEDController* ledController;
    CloudState state;

    // HTTP client for provisioning
    WiFiClientSecure wifiClient;

    // MQTT over WebSocket (512 byte buffer for larger messages)
    WebSocketsClient wsClient;
    arduino::mqtt::PubSubClient<512> mqtt;
    bool mqttConnected;
    unsigned long lastReconnectAttempt;
    unsigned long lastStatusPublish;
    int consecutiveFailures;
    bool inLongBackoff;

    // Pairing state
    bool pairingActive;
    String pairingCode;
    String pairingSessionId;
    unsigned long pairingStartTime;
    unsigned long lastPairingPoll;
    String currentApiUrl;

    // Command callback
    CloudCommandCallback commandCallback;

    // Internal methods
    bool registerForPairing();
    bool pollPairingStatus();
    void handleCredentialsReceived(const char* mqttUrl, const char* username, const char* password);
    void handleCommand(JsonObject& command);
    void setupMqttCallbacks();
};

#endif // CLOUD_MANAGER_H

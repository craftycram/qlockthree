#include "cloud_manager.h"
#include "led_controller.h"

// Pairing timeout: 10 minutes
static const unsigned long PAIRING_TIMEOUT_MS = 10 * 60 * 1000;
// Poll interval during pairing: 3 seconds
static const unsigned long PAIRING_POLL_INTERVAL_MS = 3000;
// Status publish interval: 30 seconds
static const unsigned long STATUS_PUBLISH_INTERVAL_MS = 30000;
// Reconnect interval: 5 seconds
static const unsigned long RECONNECT_INTERVAL_MS = 5000;
// Long backoff after failures: 10 minutes
static const unsigned long LONG_BACKOFF_MS = 10 * 60 * 1000;
// Max consecutive failures before long backoff
static const int MAX_CONSECUTIVE_FAILURES = 5;

CloudManager::CloudManager() :
    ledController(nullptr),
    state(CloudState::DISCONNECTED),
    mqttConnected(false),
    lastReconnectAttempt(0),
    lastStatusPublish(0),
    consecutiveFailures(0),
    inLongBackoff(false),
    pairingActive(false),
    pairingCode(""),
    pairingSessionId(""),
    pairingStartTime(0),
    lastPairingPoll(0),
    commandCallback(nullptr) {
    // Allow insecure HTTPS connections (skip certificate verification)
    wifiClient.setInsecure();
}

void CloudManager::begin(LEDController* led) {
    ledController = led;
    config.begin();

    Serial.println("CloudManager initialized");
    Serial.printf("Device ID: %s\n", DeviceIdentity::getDeviceId().c_str());

    // Initialize MQTT client with WebSocket transport
    mqtt.begin(wsClient);

    // Setup MQTT callbacks
    setupMqttCallbacks();

    // Check if already configured
    if (config.isConfigured()) {
        Serial.println("Cloud credentials found - will attempt connection");
        state = CloudState::DISCONNECTED;
    }
}

void CloudManager::setupMqttCallbacks() {
    // Callbacks are set up per-topic when subscribing
}

void CloudManager::loop() {
    // Handle pairing flow
    if (pairingActive) {
        // Check timeout
        if (millis() - pairingStartTime > PAIRING_TIMEOUT_MS) {
            Serial.println("Pairing timeout");
            stopPairing();
            return;
        }

        // Poll for pairing status
        if (millis() - lastPairingPoll > PAIRING_POLL_INTERVAL_MS) {
            lastPairingPoll = millis();
            if (pollPairingStatus()) {
                // Pairing complete - credentials received
                pairingActive = false;
                consecutiveFailures = 0;
                inLongBackoff = false;
                lastReconnectAttempt = 0;  // Allow immediate connection attempt
                state = CloudState::DISCONNECTED; // Will connect on next loop
            }
        }
        return;
    }

    // Handle normal operation
    if (!config.isConfigured()) {
        return; // Not configured, nothing to do
    }

    // MQTT update - must be called regularly
    bool wasConnected = mqtt.isConnected();
    bool wsConnected = wsClient.isConnected();
    mqtt.update();

    // Debug: check connection states
    bool wsStillConnected = wsClient.isConnected();
    if (wasConnected && !wsStillConnected) {
        Serial.println("WebSocket connection dropped!");
    }
    if (wsConnected && !wsStillConnected) {
        Serial.println("WebSocket was connected before update(), now disconnected!");
    }

    // Connection management
    if (!mqtt.isConnected()) {
        if (wasConnected) {
            Serial.printf("MQTT connection lost during update() - error: %d\n", mqtt.getLastError());
        }
        mqttConnected = false;
        if (state == CloudState::CONNECTED) {
            state = CloudState::DISCONNECTED;
            Serial.println("MQTT disconnected");
        }

        // Determine reconnect interval based on failure count
        unsigned long reconnectInterval = inLongBackoff ? LONG_BACKOFF_MS : RECONNECT_INTERVAL_MS;

        if (millis() - lastReconnectAttempt > reconnectInterval) {
            lastReconnectAttempt = millis();

            if (inLongBackoff) {
                Serial.println("Long backoff period ended, attempting reconnect...");
                inLongBackoff = false;
                consecutiveFailures = 0;
            }

            if (!connect()) {
                consecutiveFailures++;
                Serial.printf("Connection failed (%d/%d)\n", consecutiveFailures, MAX_CONSECUTIVE_FAILURES);

                if (consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) {
                    inLongBackoff = true;
                    Serial.println("Max failures reached - entering 10 minute backoff");
                }
            }
        }
        return;
    }

    // We're connected - reset failure counters
    if (!mqttConnected) {
        mqttConnected = true;
        consecutiveFailures = 0;
        inLongBackoff = false;
        state = CloudState::CONNECTED;
        Serial.println("MQTT connected!");

        // Subscribe to command topic with callback
        String commandTopic = "qlockthree/" + DeviceIdentity::getDeviceId() + "/command";
        Serial.printf("Subscribing to topic: %s\n", commandTopic.c_str());
        bool subResult = mqtt.subscribe(commandTopic, [this](const char* payload, const size_t size) {
            Serial.println(">>> MQTT CALLBACK INVOKED <<<");
            Serial.printf("Command received (%d bytes): %s\n", size, payload);

            // Parse command
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload, size);
            if (!error) {
                JsonObject command = doc.as<JsonObject>();
                handleCommand(command);
            } else {
                Serial.printf("JSON parse error: %s\n", error.c_str());
            }
        });
        Serial.printf("Subscribe result: %s, topic: %s\n", subResult ? "SUCCESS" : "FAILED", commandTopic.c_str());

        // Publish initial status
        publishStatus();
    }

    // Periodic status publishing
    if (millis() - lastStatusPublish > STATUS_PUBLISH_INTERVAL_MS) {
        lastStatusPublish = millis();
        publishStatus();
    }
}

bool CloudManager::connect() {
    if (!config.isConfigured()) {
        Serial.println("Cloud not configured - cannot connect");
        return false;
    }

    state = CloudState::CONNECTING;
    Serial.println("Connecting to MQTT over WebSocket...");

    CloudSettings settings = config.load();

    // Parse MQTT URL to extract host and port
    // Expected format: wss://host:port/path or ws://host:port/path
    String mqttUrl = String(settings.mqttUrl);
    Serial.printf("MQTT URL: %s\n", mqttUrl.c_str());

    // Extract host, port, and path from URL
    String host;
    uint16_t port = 443;
    String path = "/mqtt";

    if (mqttUrl.startsWith("wss://")) {
        mqttUrl = mqttUrl.substring(6);  // Remove "wss://"
    } else if (mqttUrl.startsWith("ws://")) {
        mqttUrl = mqttUrl.substring(5);  // Remove "ws://"
        port = 80;
    }

    // Find path
    int pathIdx = mqttUrl.indexOf('/');
    if (pathIdx > 0) {
        path = mqttUrl.substring(pathIdx);
        mqttUrl = mqttUrl.substring(0, pathIdx);
    }

    // Find port
    int portIdx = mqttUrl.indexOf(':');
    if (portIdx > 0) {
        port = mqttUrl.substring(portIdx + 1).toInt();
        host = mqttUrl.substring(0, portIdx);
    } else {
        host = mqttUrl;
    }

    Serial.printf("Connecting to: %s:%d%s\n", host.c_str(), port, path.c_str());
    Serial.printf("Username: %s\n", settings.mqttUsername);

    // Disconnect any existing connection
    mqtt.disconnect();
    wsClient.disconnect();
    delay(100);  // Allow clean disconnect

    // Begin SSL WebSocket connection
    // Empty fingerprint should trigger setInsecure() in the library
    Serial.println("Starting WebSocket SSL connection (insecure mode)...");
    wsClient.beginSSL(host.c_str(), port, path.c_str(), "", "mqtt");
    wsClient.setReconnectInterval(2000);
    wsClient.enableHeartbeat(15000, 3000, 2);  // Ping every 15s, timeout 3s, 2 retries

    // Wait for WebSocket to connect (with timeout)
    Serial.println("Waiting for WebSocket connection...");
    unsigned long wsStartTime = millis();
    const unsigned long WS_TIMEOUT = 10000;  // 10 second timeout

    while (!wsClient.isConnected() && (millis() - wsStartTime < WS_TIMEOUT)) {
        wsClient.loop();
        delay(100);
    }

    if (!wsClient.isConnected()) {
        Serial.println("WebSocket connection timeout");
        state = CloudState::DISCONNECTED;
        return false;
    }

    Serial.println("WebSocket connected, waiting before MQTT handshake...");
    delay(500);  // Give WebSocket time to stabilize

    // Connect to MQTT broker
    String clientId = "qlockthree-" + DeviceIdentity::getDeviceId();
    Serial.printf("Client ID: %s\n", clientId.c_str());
    Serial.printf("Attempting MQTT connect with user: %s\n", settings.mqttUsername);

    if (mqtt.connect(clientId.c_str(), settings.mqttUsername, settings.mqttPassword)) {
        Serial.println("MQTT connected successfully!");
        // Don't set mqttConnected here - let loop() handle it after subscribing
        state = CloudState::CONNECTING;
        return true;
    } else {
        Serial.printf("MQTT connection failed - error code: %d\n", mqtt.getLastError());
        state = CloudState::DISCONNECTED;
        return false;
    }
}

void CloudManager::disconnect() {
    mqtt.disconnect();
    wsClient.disconnect();
    mqttConnected = false;
    state = CloudState::DISCONNECTED;
    Serial.println("Disconnected from cloud");
}

bool CloudManager::isConnected() {
    return mqttConnected && mqtt.isConnected();
}

CloudState CloudManager::getState() {
    return state;
}

bool CloudManager::startPairing(const char* apiUrl) {
    if (pairingActive) {
        Serial.println("Pairing already in progress");
        return false;
    }

    currentApiUrl = String(apiUrl);
    config.setApiUrl(apiUrl);

    // Generate pairing code
    pairingCode = DeviceIdentity::generatePairingCode(6);
    Serial.printf("Generated pairing code: %s\n", pairingCode.c_str());

    // Register with backend
    if (!registerForPairing()) {
        Serial.println("Failed to register for pairing");
        return false;
    }

    pairingActive = true;
    pairingStartTime = millis();
    lastPairingPoll = 0;
    state = CloudState::PAIRING;

    Serial.println("Pairing started - waiting for user to enter code");

    return true;
}

void CloudManager::stopPairing() {
    pairingActive = false;
    pairingCode = "";
    pairingSessionId = "";
    state = config.isConfigured() ? CloudState::DISCONNECTED : CloudState::DISCONNECTED;
    Serial.println("Pairing stopped");
}

bool CloudManager::isPairing() {
    return pairingActive;
}

String CloudManager::getPairingCode() {
    return pairingCode;
}

int CloudManager::getPairingTimeRemaining() {
    if (!pairingActive) return 0;

    unsigned long elapsed = millis() - pairingStartTime;
    if (elapsed >= PAIRING_TIMEOUT_MS) return 0;

    return (PAIRING_TIMEOUT_MS - elapsed) / 1000;
}

bool CloudManager::registerForPairing() {
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate verification
    client.setHandshakeTimeout(30);  // 30 second handshake timeout

    String url = currentApiUrl + "/api/provision/start";

    Serial.println("=== PAIRING DEBUG ===");
    Serial.printf("URL: %s\n", url.c_str());
    Serial.printf("Device ID: %s\n", DeviceIdentity::getDeviceId().c_str());
    Serial.printf("Pairing Code: %s\n", pairingCode.c_str());

    http.begin(client, url);
    http.setReuse(false);  // Don't reuse connections
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(15000);  // 15 second timeout

    // Create request body
    JsonDocument doc;
    doc["deviceId"] = DeviceIdentity::getDeviceId();
    doc["code"] = pairingCode;

    String body;
    serializeJson(doc, body);
    Serial.printf("Request body: %s\n", body.c_str());

    Serial.println("Sending POST request...");
    int httpCode = http.POST(body);
    Serial.printf("HTTP response code: %d\n", httpCode);

    if (httpCode > 0) {
        String response = http.getString();
        Serial.printf("Response body: %s\n", response.c_str());

        if (httpCode == 200 || httpCode == 201) {
            JsonDocument respDoc;
            DeserializationError error = deserializeJson(respDoc, response);

            if (!error) {
                pairingSessionId = respDoc["sessionId"].as<String>();
                Serial.printf("Session ID: %s\n", pairingSessionId.c_str());

                if (respDoc["mqttUrl"].is<const char*>()) {
                    String mqttUrl = respDoc["mqttUrl"].as<String>();
                    CloudSettings settings = config.load();
                    strncpy(settings.mqttUrl, mqttUrl.c_str(), sizeof(settings.mqttUrl) - 1);
                    config.save(settings);
                }
                http.end();
                Serial.println("=== PAIRING SUCCESS ===");
                return true;
            } else {
                Serial.printf("JSON parse error: %s\n", error.c_str());
            }
        }
    } else {
        Serial.printf("HTTP error: %s\n", http.errorToString(httpCode).c_str());
    }

    Serial.println("=== PAIRING FAILED ===");
    http.end();
    return false;
}

bool CloudManager::pollPairingStatus() {
    HTTPClient http;
    String url = currentApiUrl + "/api/provision/status/" + pairingCode;

    http.begin(wifiClient, url);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String response = http.getString();

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);

        if (!error) {
            String status = doc["status"].as<String>();

            if (status == "claimed") {
                Serial.println("Device claimed! Processing credentials...");

                // Extract credentials from response
                if (doc["mqttUsername"].is<const char*>() && doc["mqttPassword"].is<const char*>()) {
                    String mqttUrl = doc["mqttUrl"].as<String>();
                    String mqttUsername = doc["mqttUsername"].as<String>();
                    String mqttPassword = doc["mqttPassword"].as<String>();

                    Serial.printf("Received MQTT credentials: %s@%s\n", mqttUsername.c_str(), mqttUrl.c_str());

                    // Save credentials
                    config.setMqttCredentials(mqttUrl.c_str(), mqttUsername.c_str(), mqttPassword.c_str());
                    config.setPaired(true);
                    config.setCloudEnabled(true);

                    Serial.println("Credentials saved successfully!");
                } else {
                    Serial.println("WARNING: No credentials in response");
                    config.setPaired(true);
                    config.setCloudEnabled(true);
                }

                http.end();
                return true;
            } else if (status == "expired") {
                Serial.println("Pairing code expired");
                stopPairing();
            }
        }
    }

    http.end();
    return false;
}

void CloudManager::handleCredentialsReceived(const char* mqttUrl, const char* username, const char* password) {
    Serial.println("Received MQTT credentials");

    config.setMqttCredentials(mqttUrl, username, password);
    config.setPaired(true);
    config.setCloudEnabled(true);

    pairingActive = false;
    consecutiveFailures = 0;
    inLongBackoff = false;
    lastReconnectAttempt = 0;  // Allow immediate connection attempt
    state = CloudState::DISCONNECTED; // Will reconnect with new credentials
}

void CloudManager::publishStatus() {
    if (!mqtt.isConnected()) return;

    String statusTopic = "qlockthree/" + DeviceIdentity::getDeviceId() + "/status";

    // Build status JSON
    JsonDocument doc;
    doc["deviceId"] = DeviceIdentity::getDeviceId();

    // Get actual state from LED controller
    if (ledController != nullptr) {
        LEDPattern pattern = ledController->getCurrentPattern();
        doc["powerState"] = (pattern == LEDPattern::OFF) ? "OFF" : "ON";
        doc["brightness"] = ledController->getBrightness();
        CRGB color = ledController->getSolidColor();
        doc["colorR"] = color.r;
        doc["colorG"] = color.g;
        doc["colorB"] = color.b;

        // Convert pattern to string
        switch (pattern) {
            case LEDPattern::OFF: doc["pattern"] = "OFF"; break;
            case LEDPattern::SOLID_COLOR: doc["pattern"] = "SOLID_COLOR"; break;
            case LEDPattern::RAINBOW: doc["pattern"] = "RAINBOW"; break;
            case LEDPattern::BREATHING: doc["pattern"] = "BREATHING"; break;
            case LEDPattern::CLOCK_DISPLAY: doc["pattern"] = "CLOCK_DISPLAY"; break;
            default: doc["pattern"] = "UNKNOWN"; break;
        }
    } else {
        // Fallback if no LED controller
        doc["powerState"] = "ON";
        doc["brightness"] = 128;
        doc["colorR"] = 255;
        doc["colorG"] = 220;
        doc["colorB"] = 180;
        doc["pattern"] = "CLOCK_DISPLAY";
    }

    doc["firmwareVersion"] = "1.0.0";
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["rssi"] = WiFi.RSSI();
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);

    mqtt.publish(statusTopic, payload);
    Serial.printf("Published status to %s\n", statusTopic.c_str());
}

void CloudManager::handleCommand(JsonObject& command) {
    String type = command["type"].as<String>();

    CloudCommandType cmdType = CloudCommandType::UNKNOWN;

    if (type == "power") {
        cmdType = CloudCommandType::POWER;
    } else if (type == "brightness") {
        cmdType = CloudCommandType::BRIGHTNESS;
    } else if (type == "color") {
        cmdType = CloudCommandType::COLOR;
    } else if (type == "pattern") {
        cmdType = CloudCommandType::PATTERN;
    }

    JsonObject payload = command["payload"].as<JsonObject>();

    if (commandCallback != nullptr) {
        commandCallback(cmdType, payload);
    }
}

void CloudManager::setCommandCallback(CloudCommandCallback callback) {
    commandCallback = callback;
}

String CloudManager::getStatusJSON() {
    JsonDocument doc;

    doc["deviceId"] = DeviceIdentity::getDeviceId();
    doc["state"] = static_cast<int>(state);

    switch (state) {
        case CloudState::DISCONNECTED:
            doc["stateText"] = "disconnected";
            break;
        case CloudState::CONNECTING:
            doc["stateText"] = "connecting";
            break;
        case CloudState::CONNECTED:
            doc["stateText"] = "connected";
            break;
        case CloudState::PAIRING:
            doc["stateText"] = "pairing";
            break;
        case CloudState::ERROR:
            doc["stateText"] = "error";
            break;
    }

    doc["connected"] = mqttConnected && mqtt.isConnected();
    doc["configured"] = config.isConfigured();
    doc["paired"] = config.isPaired();
    doc["pairingActive"] = pairingActive;

    if (pairingActive) {
        doc["pairingCode"] = pairingCode;
        doc["pairingTimeRemaining"] = getPairingTimeRemaining();
    }

    String output;
    serializeJson(doc, output);
    return output;
}

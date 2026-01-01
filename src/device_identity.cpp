#include "device_identity.h"
#include <WiFi.h>

String DeviceIdentity::cachedDeviceId = "";
bool DeviceIdentity::initialized = false;

String DeviceIdentity::getDeviceId() {
    if (!initialized || cachedDeviceId.length() == 0) {
        uint8_t mac[6];
        WiFi.macAddress(mac);

        char deviceId[20];
        snprintf(deviceId, sizeof(deviceId), "QLCK-%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        cachedDeviceId = String(deviceId);
        initialized = true;

        Serial.printf("Device ID: %s\n", cachedDeviceId.c_str());
    }

    return cachedDeviceId;
}

String DeviceIdentity::getMacAddress() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return String(macStr);
}

String DeviceIdentity::generatePairingCode(int length) {
    // Character set excludes confusing characters (0/O, 1/I/L)
    const char charset[] = "ABCDEFGHJKMNPQRSTUVWXYZ23456789";
    const int charsetSize = sizeof(charset) - 1;

    String code = "";

    // Seed random with a mix of time and MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    randomSeed(micros() + mac[0] + mac[5] * 256);

    for (int i = 0; i < length; i++) {
        code += charset[random(charsetSize)];
    }

    return code;
}

#ifndef DEVICE_IDENTITY_H
#define DEVICE_IDENTITY_H

#include <Arduino.h>

class DeviceIdentity {
public:
    // Get unique device ID based on MAC address (format: QLCK-XXXXXXXXXXXX)
    static String getDeviceId();

    // Get raw MAC address as string (format: XX:XX:XX:XX:XX:XX)
    static String getMacAddress();

    // Generate random alphanumeric pairing code
    static String generatePairingCode(int length = 6);

private:
    static String cachedDeviceId;
    static bool initialized;
};

#endif // DEVICE_IDENTITY_H

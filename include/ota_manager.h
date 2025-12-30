#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <ArduinoOTA.h>

// Forward declaration to avoid circular dependency
class LEDController;

class OTAManager {
public:
    OTAManager();
    void begin(const char* hostname, const char* password = nullptr, LEDController* ledController = nullptr);
    void handle();
    
private:
    LEDController* ledController;
    void setupCallbacks();
    void showOTAProgress(unsigned int progress, unsigned int total);
    void clearOTALEDs();
    void showOTAComplete();
};

#endif // OTA_MANAGER_H

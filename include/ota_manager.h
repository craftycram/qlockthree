#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <ArduinoOTA.h>

class OTAManager {
public:
    OTAManager();
    void begin(const char* hostname, const char* password = nullptr);
    void handle();
    
private:
    void setupCallbacks();
};

#endif // OTA_MANAGER_H
